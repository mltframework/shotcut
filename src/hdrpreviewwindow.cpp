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
#include "qmltypes/qmlutilities.h"

#include <private/qrhi_p.h>
#include <QDebug>
#include <QDir>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>

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

HdrPreviewWindow::HdrPreviewWindow(QWindow *parent)
    : QQuickView(QmlUtilities::sharedEngine(), parent)
{
    setTitle(tr("HDR Preview"));
    setResizeMode(QQuickView::SizeRootObjectToView);
    setColor(Qt::black);

    // Request HDR swapchain via the internal Qt scene graph property.
    // Qt's video fragment shaders only select the linear HDR output path
    // (nv12_bt2020_hlg_linear.frag) for HDRExtendedSrgbLinear, not for
    // HDRExtendedDisplayP3Linear. So we must use "scrgb" on all platforms.
    setProperty("_qt_sg_hdr_format", QByteArrayLiteral("scrgb"));

    rootContext()->setContextProperty("hdrWindow", this);

    QDir qmlDir = QmlUtilities::qmlDir();
    setSource(QUrl::fromLocalFile(qmlDir.filePath("views/HdrPreview.qml")));

    resize(960, 540);

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
}

void HdrPreviewWindow::pushFrame(const QVideoFrame &frame)
{
    if (m_videoSink && isVisible()) {
        if (!m_loggedSwapChain) {
            m_loggedSwapChain = true;
            auto *sc = swapChain();
            if (sc) {
                qDebug() << "HDR Preview: swapChain format =" << sc->format()
                         << "hdrInfo =" << sc->hdrInfo();
            } else {
                qDebug() << "HDR Preview: swapChain() returned nullptr!";
            }
            qDebug() << "HDR Preview frame: pixelFormat =" << frame.surfaceFormat().pixelFormat()
                     << "colorTransfer =" << frame.surfaceFormat().colorTransfer()
                     << "maxLuminance =" << frame.surfaceFormat().maxLuminance();
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
    }
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

void HdrPreviewWindow::setHlg(bool isHlg)
{
    if (m_isHlg != isHlg) {
        m_isHlg = isHlg;
        if (!m_isHlg && !qFuzzyCompare(m_hdrGain, 1.0f)) {
            m_hdrGain = 1.0f;
            emit hdrGainChanged();
        }
    }
}

void HdrPreviewWindow::updateHdrGain()
{
    if (!m_isHlg)
        return;

    auto *sc = swapChain();
    if (!sc || sc->format() != QRhiSwapChain::HDRExtendedSrgbLinear)
        return;

    auto info = sc->hdrInfo();
    float maxNits = 100.0f;
    if (info.limitsType == QRhiSwapChainHdrInfo::ColorComponentValue)
        maxNits = 100.0f * info.limits.colorComponentValue.maxColorComponentValue;
    else if (info.limitsType == QRhiSwapChainHdrInfo::LuminanceInNits)
        maxNits = info.limits.luminanceInNits.maxLuminance;

    float displayMaxLinear = maxNits / 100.0f;
    if (displayMaxLinear <= 1.0f)
        return;

    // Qt's HLG shader has a bug: maxLum is HLG-encoded (via hlgOetf) but used
    // as a linear multiplier in the OOTF.  The shader uniform is set as:
    //   maxLum = hlgOetf(maxNits / 100)
    // but it should be the linear value (maxNits / 100).  Compensate by
    // multiplying the rendered output by the ratio of the correct linear
    // value to the HLG-encoded one.
    float newGain = displayMaxLinear / hlgOetf(displayMaxLinear);
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
