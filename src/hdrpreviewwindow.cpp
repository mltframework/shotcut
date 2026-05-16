/*
 * Copyright (c) 2026 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "hdrpreviewwindow.h"

#include "actions.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "player.h"
#include "qmltypes/qmlutilities.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <private/qrhi_p.h>
#include <QDebug>
#include <QDir>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QScreen>
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include <QVulkanFunctions>
#include <QVulkanInstance>
#endif

#ifdef Q_OS_MACOS
#include "macos.h"
#endif

// HLG OETF: scene-referred linear to HLG electrical signal.
// See ITU-R BT.2100-2.
static float hlgOetf(float linear)
{
    const float a = 0.17883277f;
    const float b = 0.28466892f;
    const float c = 0.55991073f;
    if (linear < 1.0f / 12.0f)
        return sqrtf(3.0f * linear);
    return a * logf(12.0f * linear - b) + c;
}

static QString formatTimecode(int frames, double fps)
{
    if (frames < 0 || fps <= 0.0)
        return QStringLiteral("00:00");
    const int totalSec = static_cast<int>(frames / fps);
    const int h = totalSec / 3600;
    const int m = (totalSec % 3600) / 60;
    const int s = totalSec % 60;
    if (h > 0)
        return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}

HdrPreviewWindow::HdrPreviewWindow(QWindow *parent)
    : QQuickView(QmlUtilities::sharedEngine(), parent)
{
    setTitle(tr("Shotcut Preview"));
    setResizeMode(QQuickView::SizeRootObjectToView);
    setColor(Qt::black);

    // Request HDR swapchain via the internal Qt scene graph property.
    // On macOS/Windows the NVIDIA driver exposes R16G16B16A16_SFLOAT paired
    // with EXTENDED_SRGB_LINEAR_EXT, so "scrgb" works and Qt's video shaders
    // select the linear HDR output path (nv12_bt2020_hlg_linear.frag).
    // On Linux/Wayland the NVIDIA driver (as of 580.x) only offers
    // R16G16B16A16_UNORM (not SFLOAT) for that color space, making scRGB
    // impossible. Fall back to HDR10 there since A2B10G10R10_UNORM_PACK32 +
    // HDR10_ST2084_EXT IS available.
#if defined(Q_OS_LINUX)
    setProperty("_qt_sg_hdr_format", QByteArrayLiteral("hdr10"));
#else
    setProperty("_qt_sg_hdr_format", QByteArrayLiteral("scrgb"));
#endif

    rootContext()->setContextProperty("hdrWindow", this);

    QDir qmlDir = QmlUtilities::qmlDir();
    setSource(QUrl::fromLocalFile(qmlDir.filePath("views/HdrPreview.qml")));

    resize(960, 540);

    connect(this, &QWindow::windowStateChanged, this, [this]() { emit fullScreenChanged(); });

#ifdef Q_OS_MACOS
    // Override NSScreen.maximumExtendedDynamicRangeColorComponentValue so that
    // Qt's video shader outputs > 1.0 values (HDR) on the first frame, which
    // then causes macOS to allocate real EDR headroom.
    macosOverrideEdrHeadroom(true);

    // Monitor EDR headroom every second for the first 30 seconds.
    connect(&m_edrTimer, &QTimer::timeout, this, &HdrPreviewWindow::checkEdrHeadroom);
    m_edrTimer.start(1000);
#endif
}

HdrPreviewWindow::~HdrPreviewWindow()
{
#ifdef Q_OS_MACOS
    macosOverrideEdrHeadroom(false);
#endif
}

void HdrPreviewWindow::setVideoSink(QVideoSink *sink)
{
    m_videoSink = sink;
    if (m_videoSink) {
        // Force the VideoOutput's layer FBO (RGBA16F) to be cleared to a known
        // state before the first real video frame arrives. Without this, the
        // Metal/Vulkan texture backing the ShaderEffectSource may contain
        // undefined (garbage) GPU memory on its first use, producing a single
        // distorted frame on session open.
        m_videoSink->setVideoFrame(QVideoFrame());
    }
}

void HdrPreviewWindow::pushFrame(const QVideoFrame &frame)
{
    if (m_videoSink && isVisible()) {
        if (!m_loggedSwapChain) {
            m_loggedSwapChain = true;
            auto *sc = swapChain();
            if (sc) {
                qDebug() << "HDR Preview: swapChain format =" << sc->format()
                         << "hdrInfo =" << sc->hdrInfo() << "scRGB supported ="
                         << sc->isFormatSupported(QRhiSwapChain::HDRExtendedSrgbLinear)
                         << "HDR10 supported =" << sc->isFormatSupported(QRhiSwapChain::HDR10)
                         << "P3 supported ="
                         << sc->isFormatSupported(QRhiSwapChain::HDRExtendedDisplayP3Linear);
            } else {
                qDebug() << "HDR Preview: swapChain() returned nullptr!";
            }
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
            // Log Vulkan surface formats to diagnose HDR format availability
            if (auto *vi = vulkanInstance()) {
                VkSurfaceKHR surf = QVulkanInstance::surfaceForWindow(this);
                if (surf) {
                    auto *f = vi->functions();
                    VkPhysicalDevice physDev = VK_NULL_HANDLE;
                    uint32_t pdCount = 0;
                    f->vkEnumeratePhysicalDevices(vi->vkInstance(), &pdCount, nullptr);
                    if (pdCount > 0) {
                        QVarLengthArray<VkPhysicalDevice, 4> devs(pdCount);
                        f->vkEnumeratePhysicalDevices(vi->vkInstance(), &pdCount, devs.data());
                        physDev = devs[0];
                    }
                    if (physDev) {
                        auto vkGetPhysicalDeviceSurfaceFormatsKHR
                            = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
                                vi->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceFormatsKHR"));
                        if (vkGetPhysicalDeviceSurfaceFormatsKHR) {
                            uint32_t fmtCount = 0;
                            vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surf, &fmtCount, nullptr);
                            QVarLengthArray<VkSurfaceFormatKHR, 16> fmts(fmtCount);
                            vkGetPhysicalDeviceSurfaceFormatsKHR(physDev,
                                                                 surf,
                                                                 &fmtCount,
                                                                 fmts.data());
                            qDebug() << "HDR Preview: Vulkan surface has" << fmtCount << "formats:";
                            for (uint32_t i = 0; i < fmtCount; ++i) {
                                qDebug() << "  format" << fmts[i].format << "colorSpace"
                                         << fmts[i].colorSpace;
                            }
                        }
                    }
                } else {
                    qDebug() << "HDR Preview: VkSurfaceKHR is null — surface not yet created";
                }
            }
#endif
            qDebug() << "HDR Preview frame: pixelFormat =" << frame.surfaceFormat().pixelFormat()
                     << "colorTransfer =" << frame.surfaceFormat().colorTransfer()
                     << "maxLuminance =" << frame.surfaceFormat().maxLuminance();
            if (auto *scr = screen()) {
                qDebug() << "HDR Preview: screen =" << scr->name() << "size =" << scr->size()
                         << "dpr =" << scr->devicePixelRatio();
            }
#ifdef Q_OS_MACOS
            auto wid = winId();
            qDebug() << "HDR Preview EDR:"
                     << "current =" << macosCurrentEdrHeadroom(wid)
                     << "potential =" << macosPotentialEdrHeadroom(wid)
                     << "reference =" << macosReferenceEdrHeadroom(wid);
#endif
        }
        updateHdrGain();
        m_videoSink->setVideoFrame(frame);

        // Track playback position from frame timestamp
        const qint64 pts = frame.startTime(); // microseconds
        const double fps = MLT.profile().fps();
        if (pts >= 0 && fps > 0.0) {
            const int frameNum = qRound(pts * fps / 1000000.0);
            if (m_videoPosition != frameNum) {
                m_videoPosition = frameNum;
                emit videoPositionChanged();
            }
        }
        // Track duration from the active producer
        if (auto *prod = MLT.producer()) {
            const int len = qMax(0, prod->get_length() - 1);
            if (m_videoDuration != len) {
                m_videoDuration = len;
                emit videoDurationChanged();
            }
        }
    }
}

void HdrPreviewWindow::triggerPlayPause()
{
    Actions["playerPlayPauseAction"]->trigger();
}

void HdrPreviewWindow::triggerRewind()
{
    Actions["playerRewindAction"]->trigger();
}

void HdrPreviewWindow::triggerFastForward()
{
    Actions["playerFastForwardAction"]->trigger();
}

void HdrPreviewWindow::restoreGeometry(const QRect &r)
{
    // Suppress the DAR-snap in resizeEvent so that programmatically restoring
    // the saved window geometry does not trigger a floating-point-rounded
    // resize that would make the window grow by 1-2 px on every launch.
    m_skipDarSnap = true;
    setGeometry(r);
    // On macOS the resizeEvent may fire during show() rather than during
    // setGeometry(), so keep the guard active for a short while.
    QTimer::singleShot(300, this, [this]() { m_skipDarSnap = false; });
}

void HdrPreviewWindow::toggleFullScreen()
{
    if (windowStates() & Qt::WindowFullScreen) {
        setWindowStates(Qt::WindowNoState);
        if (m_normalGeometry.isValid())
            setGeometry(m_normalGeometry);
    } else {
        m_normalGeometry = geometry();
        showFullScreen();
    }
}

QString HdrPreviewWindow::positionText() const
{
    return formatTimecode(m_videoPosition, MLT.profile().fps());
}

QString HdrPreviewWindow::durationText() const
{
    return formatTimecode(m_videoDuration, MLT.profile().fps());
}

void HdrPreviewWindow::seekToFrame(int frame)
{
    MAIN.player()->seek(frame);
}

void HdrPreviewWindow::setPlaying(bool playing)
{
    if (m_isPlaying != playing) {
        m_isPlaying = playing;
        emit playingChanged();
    }
}

bool HdrPreviewWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_SIZING && !(windowStates() & Qt::WindowFullScreen)) {
            const int darNum = MLT.profile().display_aspect_num();
            const int darDen = MLT.profile().display_aspect_den();
            if (darNum > 0 && darDen > 0) {
                RECT *r = reinterpret_cast<RECT *>(msg->lParam);
                // Measure the actual frame overhead from the live window state.
                // AdjustWindowRectEx under-reports the DWM extended frame on
                // Windows 10/11, leading to a wrong client-area AR calculation.
                RECT curWin, curClient;
                GetWindowRect(msg->hwnd, &curWin);
                GetClientRect(msg->hwnd, &curClient);
                const int fw = (curWin.right - curWin.left) - (curClient.right - curClient.left);
                const int fh = (curWin.bottom - curWin.top) - (curClient.bottom - curClient.top);
                const int clientW = (r->right - r->left) - fw;
                const int clientH = (r->bottom - r->top) - fh;
                const bool heightPrimary = (msg->wParam == WMSZ_TOP || msg->wParam == WMSZ_BOTTOM);
                if (heightPrimary) {
                    // Height drives: adjust width from the right
                    r->right = r->left + qRound((double) clientH * darNum / darDen) + fw;
                } else {
                    // Width drives: adjust height
                    const int newH = qRound((double) clientW * darDen / darNum) + fh;
                    const bool fromTop = (msg->wParam == WMSZ_TOPLEFT
                                          || msg->wParam == WMSZ_TOPRIGHT);
                    if (fromTop)
                        r->top = r->bottom - newH;
                    else
                        r->bottom = r->top + newH;
                }
                *result = TRUE;
                return true;
            }
        }
    }
#endif
    return QQuickView::nativeEvent(eventType, message, result);
}

void HdrPreviewWindow::resizeEvent(QResizeEvent *event)
{
    QQuickView::resizeEvent(event);
#ifndef Q_OS_WIN
    // On Windows, WM_SIZING handles AR constraining smoothly.
    // On other platforms, snap to the correct AR after each resize.
    if (windowStates() & Qt::WindowFullScreen)
        return;
    if (m_skipDarSnap)
        return;
    const QSize newSize = event->size();
    const QSize oldSize = event->oldSize();
    if (!oldSize.isValid())
        return;
    const int darNum = MLT.profile().display_aspect_num();
    const int darDen = MLT.profile().display_aspect_den();
    if (darNum <= 0 || darDen <= 0)
        return;
    // Infer which axis the user is dragging by which changed proportionally more
    const double wChange = qAbs((double) (newSize.width() - oldSize.width())
                                / qMax(oldSize.width(), 1));
    const double hChange = qAbs((double) (newSize.height() - oldSize.height())
                                / qMax(oldSize.height(), 1));
    int targetW, targetH;
    if (wChange >= hChange) {
        targetW = newSize.width();
        targetH = qRound((double) targetW * darDen / darNum);
    } else {
        targetH = newSize.height();
        targetW = qRound((double) targetH * darNum / darDen);
    }
    if (targetW != newSize.width() || targetH != newSize.height()) {
        // Defer to avoid recursion
        QTimer::singleShot(0, this, [this, targetW, targetH]() {
            if (!(windowStates() & Qt::WindowFullScreen))
                resize(targetW, targetH);
        });
    }
#endif
}

void HdrPreviewWindow::keyPressEvent(QKeyEvent *event)
{
    // Forward to MainWindow for J/K/L transport handling
    event->setAccepted(false);
    MAIN.keyPressEvent(event);
    if (event->isAccepted())
        return;

    // Match QAction shortcuts since this window is outside MainWindow's hierarchy
    QKeySequence keySeq(event->keyCombination());
    for (const auto &key : Actions.keys()) {
        QAction *action = Actions[key];
        if (action && action->isEnabled()) {
            for (const auto &shortcut : action->shortcuts()) {
                if (shortcut.matches(keySeq) == QKeySequence::ExactMatch) {
                    action->trigger();
                    event->accept();
                    return;
                }
            }
        }
    }
}

void HdrPreviewWindow::keyReleaseEvent(QKeyEvent *event)
{
    event->setAccepted(false);
    MAIN.keyReleaseEvent(event);
    if (!event->isAccepted())
        QQuickView::keyReleaseEvent(event);
}

void HdrPreviewWindow::setHdrTransfer(HdrTransfer transfer)
{
    if (m_hdrTransfer != transfer) {
        m_hdrTransfer = transfer;
        if (m_hdrTransfer == HdrTransfer::SDR && !qFuzzyCompare(m_hdrGain, 1.0f)) {
            m_hdrGain = 1.0f;
            emit hdrGainChanged();
        }
    }
}

void HdrPreviewWindow::updateHdrGain()
{
    if (m_hdrTransfer == HdrTransfer::SDR)
        return;

    auto *sc = swapChain();
    if (!sc
        || (sc->format() != QRhiSwapChain::HDRExtendedSrgbLinear
            && sc->format() != QRhiSwapChain::HDR10)) {
        if (!m_loggedGainSkip) {
            m_loggedGainSkip = true;
            if (!sc)
                qDebug() << "HDR Preview: gain skipped — no swapChain";
            else
                qDebug() << "HDR Preview: gain skipped — swapChain format" << sc->format()
                         << "is not HDR. Try QSG_RHI_BACKEND=vulkan on Linux.";
        }
        return;
    }

    auto info = sc->hdrInfo();
    float maxNits = 100.0f;
    if (info.limitsType == QRhiSwapChainHdrInfo::ColorComponentValue)
        maxNits = 100.0f * info.limits.colorComponentValue.maxColorComponentValue;
    else if (info.limitsType == QRhiSwapChainHdrInfo::LuminanceInNits)
        maxNits = info.limits.luminanceInNits.maxLuminance;

    float displayMaxLinear = maxNits / 100.0f;
    if (displayMaxLinear <= 1.0f)
        return;

    float newGain;
    if (sc->format() == QRhiSwapChain::HDR10 || m_hdrTransfer == HdrTransfer::PQ) {
        // PQ (ST.2084) — Qt applies the PQ EOTF correctly in its shader,
        // both for HDR10 swapchains and for scRGB with ColorTransfer_ST2084.
        // No additional gain correction is needed.
        newGain = 1.0f;
    } else {
        // scRGB (HDRExtendedSrgbLinear) path with HLG content.
        // Qt's HLG shader has a bug: maxLum is HLG-encoded (via hlgOetf) but used
        // as a linear multiplier in the OOTF.  The shader uniform is set as:
        //   maxLum = hlgOetf(maxNits / 100)
        // but it should be the linear value (maxNits / 100).  Compensate by
        // multiplying the rendered output by the ratio of the correct linear
        // value to the HLG-encoded one.
        newGain = displayMaxLinear / hlgOetf(displayMaxLinear);
    }
    if (!qFuzzyCompare(newGain, m_hdrGain)) {
        m_hdrGain = newGain;
        qDebug() << "HDR Preview: gain =" << m_hdrGain << "(maxNits =" << maxNits << ")";
        emit hdrGainChanged();
    }
}

void HdrPreviewWindow::checkEdrHeadroom()
{
#ifdef Q_OS_MACOS
    float headroom = macosCurrentEdrHeadroom(winId());
    if (headroom != m_lastLoggedHeadroom) {
        m_lastLoggedHeadroom = headroom;
        auto *sc = swapChain();
        qDebug() << "HDR Preview: EDR headroom =" << headroom
                 << "(swapChain hdrInfo =" << (sc ? sc->hdrInfo() : QRhiSwapChainHdrInfo()) << ")";
    }
    if (++m_edrCheckCount >= 30)
        m_edrTimer.stop();
#endif
}
