/*
 * Copyright (c) 2011-2026 Meltytech, LLC
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

#include "videowidget.h"

#include "Logger.h"
#include "mainwindow.h"
#include "qmltypes/qmlfilter.h"
#include "qmltypes/qmlutilities.h"
#include "settings.h"
#include "util.h"

#include <Mlt.h>
#include <functional>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QQuickItem>
#include <QUrl>
#include <QtQml>
#include <QtWidgets>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#if defined(__x86_64__) || defined(_M_AMD64)
#include <immintrin.h>
#endif

using namespace Mlt;

namespace {

struct DecklinkHdrPrimaries
{
    double redX;
    double redY;
    double greenX;
    double greenY;
    double blueX;
    double blueY;
    double whiteX;
    double whiteY;
};

DecklinkHdrPrimaries decklinkHdrPrimaries(int preset)
{
    if (preset == 1)
        return {0.6800, 0.3200, 0.2650, 0.6900, 0.1500, 0.0600, 0.3127, 0.3290};
    return {0.7080, 0.2920, 0.1700, 0.7970, 0.1310, 0.0460, 0.3127, 0.3290};
}

} // namespace

VideoWidget::VideoWidget(QObject *parent)
    : QQuickWidget(QmlUtilities::sharedEngine(), (QWidget *) parent)
    , Controller()
    , m_grid(0)
    , m_initSem(0)
    , m_isInitialized(false)
    , m_frameSemaphore(3)
    , m_imageRequested(false)
    , m_oldVideoOutput(Settings.playerOldVideoOutput())
    , m_frameRenderer(nullptr)
    , m_zoom(0.0f)
    , m_offset(QPoint(0, 0))
    , m_snapToGrid(true)
    , m_scrubAudio(false)
    , m_maxTextureSize(4096)
    , m_hideVui(false)
    , m_p016Pool(std::make_shared<P016Pool>())
{
    LOG_DEBUG() << "begin";
    setAttribute(Qt::WA_AcceptTouchEvents);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setClearColor(palette().window().color());
    QDir importPath = QmlUtilities::qmlDir();
    importPath.cd("modules");
    engine()->addImportPath(importPath.path());
    QmlUtilities::setCommonProperties(rootContext());
    rootContext()->setContextProperty("video", this);
    m_refreshTimer.setInterval(10);
    m_refreshTimer.setSingleShot(true);

    if (Settings.playerGPU())
        m_glslManager.reset(new Filter(profile(), "glsl.manager"));
    if ((m_glslManager && !m_glslManager->is_valid())) {
        m_glslManager.reset();
    }

    connect(quickWindow(),
            &QQuickWindow::visibilityChanged,
            this,
            &VideoWidget::setBlankScene,
            Qt::QueuedConnection);
    connect(&m_refreshTimer, &QTimer::timeout, this, &VideoWidget::onRefreshTimeout);
    connect(this, &VideoWidget::rectChanged, this, &VideoWidget::zoomChanged);
    LOG_DEBUG() << "end";
}

VideoWidget::~VideoWidget()
{
    LOG_DEBUG() << "begin";
    stop();
    if (m_frameRenderer && m_frameRenderer->isRunning()) {
        m_frameRenderer->quit();
        m_frameRenderer->wait();
        m_frameRenderer->deleteLater();
    }
    LOG_DEBUG() << "end";
}

void VideoWidget::initialize()
{
    LOG_DEBUG() << "begin";
    if (m_oldVideoOutput) {
        m_frameRenderer = new FrameRenderer();
        connect(m_frameRenderer,
                &FrameRenderer::frameDisplayed,
                this,
                &VideoWidget::onFrameDisplayed,
                Qt::QueuedConnection);
        connect(m_frameRenderer,
                &FrameRenderer::frameDisplayed,
                this,
                &VideoWidget::frameDisplayed,
                Qt::QueuedConnection);
        connect(m_frameRenderer, &FrameRenderer::imageReady, this, &VideoWidget::imageReady);
    }
    m_initSem.release();
    m_isInitialized = true;
    LOG_DEBUG() << "end";
}

void VideoWidget::setBlankScene()
{
    quickWindow()->setColor(palette().window().color());
    setSource(QmlUtilities::blankVui());
    m_savedQmlSource.clear();
}

void VideoWidget::resizeVideo(int width, int height)
{
    double x, y, w, h;
    double this_aspect = (double) width / height;
    double video_aspect = profile().dar();

    // Special case optimisation to negate odd effect of sample aspect ratio
    // not corresponding exactly with image resolution.
    if ((int) (this_aspect * 1000) == (int) (video_aspect * 1000)) {
        w = width;
        h = height;
    }
    // Use OpenGL to normalise sample aspect ratio
    else if (height * video_aspect > width) {
        w = width;
        h = width / video_aspect;
    } else {
        w = height * video_aspect;
        h = height;
    }
    x = (width - w) / 2.0;
    y = (height - h) / 2.0;
    m_rect.setRect(x, y, w, h);
    emit rectChanged();
}

void VideoWidget::resizeEvent(QResizeEvent *event)
{
    QQuickWidget::resizeEvent(event);
    resizeVideo(event->size().width(), event->size().height());
}

void VideoWidget::onRefreshTimeout()
{
    Controller::refreshConsumer(m_scrubAudio);
    m_scrubAudio = false;
}

void VideoWidget::mousePressEvent(QMouseEvent *event)
{
    QQuickWidget::mousePressEvent(event);
    if (event->isAccepted())
        return;
    if (event->button() == Qt::LeftButton)
        m_dragStart = event->pos();
    else if (event->button() == Qt::MiddleButton)
        m_mousePosition = event->pos();
    if (MLT.isClip())
        emit dragStarted();
}

void VideoWidget::mouseMoveEvent(QMouseEvent *event)
{
    QQuickWidget::mouseMoveEvent(event);
    if (event->isAccepted())
        return;
    if (event->buttons() & Qt::MiddleButton) {
        emit offsetChanged(m_offset + m_mousePosition - event->pos());
        m_mousePosition = event->pos();
        return;
    }
    if (event->modifiers() == (Qt::ShiftModifier | Qt::AltModifier) && m_producer) {
        emit seekTo(m_producer->get_length() * event->position().x() / width());
        return;
    }
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if (m_dragStart.isNull())
        return;
    if ((event->pos() - m_dragStart).manhattanLength() < QApplication::startDragDistance())
        return;
    // Reset the drag point to prevent repeating drag actions.
    m_dragStart.setX(0);
    m_dragStart.setY(0);
    if (!MLT.producer())
        return;
    if (MLT.isMultitrack() || MLT.isPlaylist()) {
        MAIN.showStatusMessage(tr("You cannot drag from Project."));
        return;
    } else if (!MLT.isSeekableClip()) {
        MAIN.showStatusMessage(tr("You cannot drag a non-seekable source"));
        return;
    }

    // Cannot show a DurationDialog during mouse drag
    // This is usually MLT.isLiveProducer(), but that checks for > 1 week,
    // which is too long for the timeline.
    if (m_producer->get_playtime() > qRound(profile().fps() * 24 * 3600)) {
        m_producer->set_in_and_out(0, profile().fps() * 60 - 1);
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(Mlt::XmlMimeType, MLT.XML().toUtf8());
    drag->setMimeData(mimeData);
    mimeData->setText(QString::number(MLT.producer()->get_playtime()));
    SharedFrame sourceFrame = m_sharedFrame.is_valid()
                                  ? m_sharedFrame
                                  : (m_frameRenderer ? m_frameRenderer->getDisplayFrame()
                                                     : SharedFrame());
    if (sourceFrame.is_valid()) {
        constexpr int kDragThumbnailHeight = 45;
        Mlt::Frame displayFrame(sourceFrame.clone(false, true));
        QImage displayImage = MLT.image(&displayFrame,
                                        kDragThumbnailHeight * MLT.profile().dar(),
                                        kDragThumbnailHeight)
                                  .scaledToHeight(kDragThumbnailHeight);
        drag->setPixmap(QPixmap::fromImage(displayImage));
    }
    drag->setHotSpot(QPoint(0, 0));
    drag->exec(Qt::CopyAction);
}

void VideoWidget::renderVideo() {}

void VideoWidget::onFrameDisplayed(const SharedFrame &frame)
{
    m_mutex.lock();
    m_sharedFrame = frame;
    m_mutex.unlock();
    bool isVui = frame.get_int(kShotcutVuiMetaProperty) && !m_hideVui;
    if (!isVui && source() != QmlUtilities::blankVui()) {
        m_savedQmlSource = source();
        setSource(QmlUtilities::blankVui());
    } else if (isVui && !m_savedQmlSource.isEmpty() && source() != m_savedQmlSource) {
        setSource(m_savedQmlSource);
    }
    quickWindow()->update();
}

void VideoWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ShiftModifier) {
        float step = event->angleDelta().y();
        if (step == 0.0) {
            step = event->angleDelta().x();
        }
        if (step != 0.0) {
            // Convert to degrees
            step = step / 8.0 / 15.0;
            // Convert to zoom factor step (increments of 0.05)
            step = step * 0.05;
            // Calculate the zoom level that would fit
            float estimatedFitX = width() / MLT.profile().width();
            float estimatedFitY = height() / MLT.profile().height();
            float estimatedFit = estimatedFitX;
            if (estimatedFit > estimatedFitY) {
                estimatedFit = estimatedFitY;
            }
            emit stepZoom(step, estimatedFit);
            event->accept();
        }
    } else {
        QQuickWidget::wheelEvent(event);
    }
}

void VideoWidget::keyPressEvent(QKeyEvent *event)
{
    QQuickWidget::keyPressEvent(event);
    if (event->isAccepted())
        return;
    MAIN.keyPressEvent(event);
}

bool VideoWidget::event(QEvent *event)
{
    bool result = QQuickWidget::event(event);
    if (event->type() == QEvent::PaletteChange) {
        if (m_frameRenderer && m_sharedFrame.is_valid())
            onFrameDisplayed(m_sharedFrame);
        else
            setClearColor(palette().window().color());
    }
    return result;
}

int VideoWidget::setProducer(Mlt::Producer *producer, bool isMulti)
{
    int error = Controller::setProducer(producer, isMulti);

    if (!error) {
        error = reconfigure(isMulti);
        if (!error) {
            // The profile display aspect ratio may have changed.
            resizeVideo(width(), height());
        }
    }
    return error;
}

void VideoWidget::createThread(RenderThread **thread, thread_function_t function, void *data)
{
#ifdef Q_OS_WIN
    // On Windows, MLT event consumer-thread-create is fired from the Qt main thread.
    while (!m_isInitialized)
        QCoreApplication::processEvents();
#else
    if (!m_isInitialized) {
        m_initSem.acquire();
    }
#endif
    if (!m_renderThread) {
        m_renderThread.reset(new RenderThread(function, data));
        (*thread) = m_renderThread.get();
        (*thread)->start();
    } else {
        m_renderThread->start();
    }
}

static void onThreadCreate(mlt_properties owner, VideoWidget *self, mlt_event_data data)
{
    Q_UNUSED(owner)
    auto threadData = (mlt_event_data_thread *) Mlt::EventData(data).to_object();
    if (threadData) {
        self->createThread((RenderThread **) threadData->thread,
                           threadData->function,
                           threadData->data);
    }
}

static void onThreadJoin(mlt_properties owner, VideoWidget *self, mlt_event_data data)
{
    Q_UNUSED(owner)
    Q_UNUSED(self)
    auto threadData = (mlt_event_data_thread *) Mlt::EventData(data).to_object();
    if (threadData && threadData->thread) {
        auto renderThread = (RenderThread *) *threadData->thread;
        if (renderThread) {
            renderThread->quit();
            renderThread->wait();
        }
    }
}

void VideoWidget::startGlsl()
{
    if (m_glslManager) {
        m_glslManager->fire_event("init glsl");
        if (!m_glslManager->get_int("glsl_supported")) {
            m_glslManager.reset();
            // Need to destroy MLT global reference to prevent filters from trying to use GPU.
            mlt_properties_clear(mlt_global_properties(), "glslManager");
            emit gpuNotSupported();
        } else {
            emit started();
        }
    }
}

static void onThreadStarted(mlt_properties owner, VideoWidget *self)
{
    Q_UNUSED(owner)
    self->startGlsl();
}

void VideoWidget::stopGlsl()
{
    //TODO This is commented out for now because it is causing crashes.
    //Technically, this should be the correct thing to do, but it appears
    //some changes in the 15.01 and 15.03 releases have created regression
    //with respect to restarting the consumer in GPU mode.
    //    m_glslManager->fire_event("close glsl");
}

static void onThreadStopped(mlt_properties owner, VideoWidget *self)
{
    Q_UNUSED(owner)
    self->stopGlsl();
}

int VideoWidget::reconfigure(bool isMulti)
{
    int error = 0;

    // use SDL for audio, OpenGL for video
    QString serviceName = property("mlt_service").toString();
    if (!m_consumer || !m_consumer->is_valid()) {
        if (serviceName.isEmpty()) {
            m_consumer.reset(new Mlt::FilteredConsumer(previewProfile(), "sdl2_audio"));
            if (m_consumer->is_valid())
                serviceName = "sdl2_audio";
            else
                serviceName = "rtaudio";
            m_consumer.reset();
        }
        if (isMulti)
            m_consumer.reset(new Mlt::FilteredConsumer(previewProfile(), "multi"));
        else
            m_consumer.reset(
                new Mlt::FilteredConsumer(previewProfile(), serviceName.toLatin1().constData()));

        m_threadStartEvent.reset();
        m_threadStopEvent.reset();
        m_threadCreateEvent.reset();
        m_threadJoinEvent.reset();
    }
    if (m_consumer->is_valid()) {
        // Connect the producer to the consumer - tell it to "run" later
        if (m_producer && m_producer->is_valid())
            m_consumer->connect(*m_producer);
        // Make an event handler for when a frame's image should be displayed
        m_consumer->listen("consumer-frame-show", this, (mlt_listener) on_frame_show);
        m_consumer->set("real_time", MLT.realTime());
        m_consumer->set("scale", double(Settings.playerPreviewScale()) / MLT.profile().height());
        const int processingMode = property("processing_mode").toInt();
        const QString profileTrc = MLT.colorTrc();
        const bool hdrPreview = MLT.isHDR() && property("hdr_preview").toBool();
        const bool isDeckLinkHdr = serviceName.startsWith("decklink") && MLT.isHDR();
        switch (processingMode) {
        case ShotcutSettings::Linear10Cpu:
            m_consumer->set("mlt_image_format", "rgba64");
            break;
        case ShotcutSettings::Native10Cpu:
        case ShotcutSettings::Linear10GpuCpu:
            m_consumer->set("mlt_image_format",
                            isDeckLinkHdr ? "yuv444p10"
                            : hdrPreview  ? "yuv420p10"
                                          : "rgba64");
            break;
        default: // Native8Cpu
            m_consumer->set("mlt_image_format",
                            serviceName.startsWith("decklink") ? "yuv422" : "yuv420p");
            break;
        }
        m_consumer->set("channels", property("audio_channels").toInt());
        if (property("audio_channels").toInt() == 4) {
            m_consumer->set("channel_layout", "quad");
        } else {
            m_consumer->set("channel_layout", "auto");
        }
        // Follow the active video mode transfer when it is explicit; otherwise,
        // fall back to colorspace-based SDR defaults.
        if (MLT.isHDR()) {
            m_consumer->set("color_trc", profileTrc.toLatin1().constData());
        } else {
            switch (MLT.profile().colorspace()) {
            case 601:
            case 170:
                m_consumer->set("color_trc", "smpte170m");
                break;
            case 240:
                m_consumer->set("color_trc", "smpte240m");
                break;
            case 470:
                m_consumer->set("color_trc", "bt470bg");
                break;
            default:
                m_consumer->set("color_trc", "bt709");
                break;
            }
        }
        const char *activeTrc = m_consumer->get("color_trc");
        HdrTransfer hdrTransfer = hdrTransferFromTrc(QLatin1String(activeTrc));
        emit hdrTransferChanged(hdrTransfer);
        if (processingMode == ShotcutSettings::Linear10Cpu
            || (processingMode == ShotcutSettings::Linear10GpuCpu && !MLT.isHDR())) {
            m_consumer->set("mlt_color_trc", "linear");
        } else {
            m_consumer->clear("mlt_color_trc");
        }
        LOG_DEBUG() << "mlt_image_format" << m_consumer->get("mlt_image_format") << "mlt_color_trc"
                    << m_consumer->get("mlt_color_trc") << "color_trc"
                    << m_consumer->get("color_trc");

        if (serviceName.startsWith("decklink")) {
            const QString prefix = isMulti ? QStringLiteral("0.") : QString();
            const DecklinkHdrPrimaries primaries = decklinkHdrPrimaries(
                property("decklinkHdrMasterPreset").toInt());
            const auto setConsumerInt = [&](const QString &name, int value) {
                m_consumer->set(name.toLatin1().constData(), value);
            };
            const auto setConsumerDouble = [&](const QString &name, double value) {
                m_consumer->set(name.toLatin1().constData(), value);
            };
            setConsumerDouble(prefix + QStringLiteral("hdr_red_x"), primaries.redX);
            setConsumerDouble(prefix + QStringLiteral("hdr_red_y"), primaries.redY);
            setConsumerDouble(prefix + QStringLiteral("hdr_green_x"), primaries.greenX);
            setConsumerDouble(prefix + QStringLiteral("hdr_green_y"), primaries.greenY);
            setConsumerDouble(prefix + QStringLiteral("hdr_blue_x"), primaries.blueX);
            setConsumerDouble(prefix + QStringLiteral("hdr_blue_y"), primaries.blueY);
            setConsumerDouble(prefix + QStringLiteral("hdr_white_x"), primaries.whiteX);
            setConsumerDouble(prefix + QStringLiteral("hdr_white_y"), primaries.whiteY);
            setConsumerInt(prefix + QStringLiteral("hdr_max_cll"),
                           property("decklinkHdrMaxCll").toInt());
            setConsumerInt(prefix + QStringLiteral("hdr_max_fall"),
                           property("decklinkHdrMaxFall").toInt());
            setConsumerInt(prefix + QStringLiteral("hdr_max_luminance"),
                           property("decklinkHdrMaxLuminance").toInt());
            setConsumerDouble(prefix + QStringLiteral("hdr_min_luminance"),
                              property("decklinkHdrMinLuminance").toDouble());
        }
        if (isMulti) {
            m_consumer->set("terminate_on_pause", 0);
            m_consumer->set("0", serviceName.toLatin1().constData());
            if (!profile().progressive())
                m_consumer->set("0.progressive", property("progressive").toBool());
            m_consumer->set("0.rescale", property("rescale").toString().toLatin1().constData());
            m_consumer->set("0.deinterlacer",
                            property("deinterlacer").toString().toLatin1().constData());
            m_consumer->set("0.buffer", qMax(25, qRound(profile().fps())));
            m_consumer->set("0.prefill", 8);
            m_consumer->set("0.drop_max", qRound(profile().fps() / 4.0));
            if (property("keyer").isValid())
                m_consumer->set("0.keyer", property("keyer").toInt());
            m_consumer->set("0.video_delay", Settings.playerVideoDelayMs());
        } else {
            if (!profile().progressive())
                m_consumer->set("progressive", property("progressive").toBool());
            m_consumer->set("rescale", property("rescale").toString().toLatin1().constData());
            m_consumer->set("deinterlacer",
                            property("deinterlacer").toString().toLatin1().constData());
            m_consumer->set("buffer", qMax(25, qRound(profile().fps())));
            m_consumer->set("prefill", 8);
            m_consumer->set("drop_max", qRound(profile().fps() / 4.0));
            if (property("keyer").isValid())
                m_consumer->set("keyer", property("keyer").toInt());
            m_consumer->set("video_delay", Settings.playerVideoDelayMs());
        }
        if (m_glslManager) {
            if (!m_threadCreateEvent)
                m_threadCreateEvent.reset(m_consumer->listen("consumer-thread-create",
                                                             this,
                                                             (mlt_listener) onThreadCreate));
            if (!m_threadJoinEvent)
                m_threadJoinEvent.reset(
                    m_consumer->listen("consumer-thread-join", this, (mlt_listener) onThreadJoin));
            if (!m_threadStartEvent)
                m_threadStartEvent.reset(m_consumer->listen("consumer-thread-started",
                                                            this,
                                                            (mlt_listener) onThreadStarted));
            if (!m_threadStopEvent)
                m_threadStopEvent.reset(m_consumer->listen("consumer-thread-stopped",
                                                           this,
                                                           (mlt_listener) onThreadStopped));
        } else {
            emit started();
        }
    } else {
        // Cleanup on error
        error = 2;
        Controller::closeConsumer();
        Controller::close();
    }
    return error;
}

void VideoWidget::refreshConsumer(bool scrubAudio)
{
    scrubAudio |= isPaused() ? scrubAudio : Settings.playerScrubAudio();
    m_scrubAudio |= scrubAudio;
    m_refreshTimer.start();
}

QPoint VideoWidget::offset() const
{
    if (m_zoom == 0.0) {
        return QPoint(0, 0);
    } else {
        double displayW = MLT.profile().height() * MLT.profile().dar();
        double displayH = MLT.profile().height();
        return QPoint(m_offset.x() - (displayW * m_zoom - width()) / 2,
                      m_offset.y() - (displayH * m_zoom - height()) / 2);
    }
}

QImage VideoWidget::image() const
{
    SharedFrame frame = m_frameRenderer ? m_frameRenderer->getDisplayFrame() : m_sharedFrame;
    if (frame.is_valid()) {
        const uint8_t *image = frame.get_image(mlt_image_rgba);
        if (image) {
            int width = frame.get_image_width();
            int height = frame.get_image_height();
            QImage temp(image, width, height, QImage::Format_RGBA8888);
            return temp.copy();
        }
    }
    return QImage();
}

bool VideoWidget::imageIsProxy() const
{
    bool isProxy = false;
    SharedFrame frame = m_frameRenderer ? m_frameRenderer->getDisplayFrame() : m_sharedFrame;
    if (frame.is_valid()) {
        Mlt::Producer *frameProducer = frame.get_original_producer();
        if (frameProducer && frameProducer->is_valid() && frameProducer->get_int(kIsProxyProperty)) {
            isProxy = true;
        }
        delete frameProducer;
    }
    return isProxy;
}

bool VideoWidget::oldVideoOutput() const
{
    return m_oldVideoOutput;
}

void VideoWidget::requestImage()
{
    if (m_frameRenderer)
        m_frameRenderer->requestImage();
    else
        m_imageRequested = true;
}

void VideoWidget::toggleVuiDisplay()
{
    m_hideVui = !m_hideVui;
    refreshConsumer();
}

void VideoWidget::setVideoSink(QVideoSink *sink)
{
    m_videoSink = sink;
    if (m_videoSink && m_sharedFrame.is_valid())
        pushFrameToSink(m_sharedFrame);
}

#if defined(__x86_64__) || defined(_M_AMD64)
#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("avx2")))
#endif
static void
shiftYPlane_AVX2(const uint16_t *src, uint16_t *dst, int n)
{
    int i = 0;
    for (; i + 16 <= n; i += 16) {
        __m256i y = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + i));
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst + i), _mm256_slli_epi16(y, 6));
    }
    for (; i < n; ++i)
        dst[i] = src[i] << 6;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("avx2")))
#endif
static void
interleaveUVPlanes_AVX2(const uint16_t *srcU, const uint16_t *srcV, uint16_t *dst, int n)
{
    // AVX2 unpack operates within 128-bit lanes; permute to restore linear order.
    // unpacklo(u,v): lane0 = u0v0u1v1u2v2u3v3, lane1 = u8v8...u11v11
    // unpackhi(u,v): lane0 = u4v4...u7v7,      lane1 = u12v12...u15v15
    // permute2x128 0x20 → [lo.lane0 | hi.lane0] = u0v0..u7v7
    // permute2x128 0x31 → [lo.lane1 | hi.lane1] = u8v8..u15v15
    int j = 0;
    for (; j + 16 <= n; j += 16) {
        __m256i u
            = _mm256_slli_epi16(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(srcU + j)), 6);
        __m256i v
            = _mm256_slli_epi16(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(srcV + j)), 6);
        __m256i lo = _mm256_unpacklo_epi16(u, v);
        __m256i hi = _mm256_unpackhi_epi16(u, v);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst + j * 2),
                            _mm256_permute2x128_si256(lo, hi, 0x20));
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst + j * 2 + 16),
                            _mm256_permute2x128_si256(lo, hi, 0x31));
    }
    for (; j < n; ++j) {
        dst[2 * j] = srcU[j] << 6;
        dst[2 * j + 1] = srcV[j] << 6;
    }
}

static void shiftYPlane_SSE2(const uint16_t *src, uint16_t *dst, int n)
{
    int i = 0;
    for (; i + 8 <= n; i += 8) {
        __m128i y = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + i), _mm_slli_epi16(y, 6));
    }
    for (; i < n; ++i)
        dst[i] = src[i] << 6;
}

static void interleaveUVPlanes_SSE2(const uint16_t *srcU, const uint16_t *srcV, uint16_t *dst, int n)
{
    int j = 0;
    for (; j + 8 <= n; j += 8) {
        __m128i u = _mm_slli_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i *>(srcU + j)), 6);
        __m128i v = _mm_slli_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i *>(srcV + j)), 6);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + j * 2), _mm_unpacklo_epi16(u, v));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + j * 2 + 8), _mm_unpackhi_epi16(u, v));
    }
    for (; j < n; ++j) {
        dst[2 * j] = srcU[j] << 6;
        dst[2 * j + 1] = srcV[j] << 6;
    }
}
#endif // defined(__x86_64__) || defined(_M_AMD64)

// Convert MLT planar yuv420p10 (Y, U, V planes; 10-bit in LSBs of uint16_t) to
// semi-planar P016 (Y plane + interleaved UV plane; 10-bit shifted to full 16-bit range).
// `buffer` is resized as needed; passing a pre-allocated buffer avoids heap allocation.
static void convertToP016(const uint8_t *image, int width, int height, QByteArray &buffer)
{
    const int uvW = width / 2;
    const int uvH = height / 2;
    const int ySamples = width * height;
    const int yPlaneSize = ySamples * 2;
    const int uvPlaneSize = uvW * uvH * 2;
    const int interleavedUvSize = uvW * uvH * 4;
    buffer.resize(yPlaneSize + interleavedUvSize);
    const uint16_t *srcY = reinterpret_cast<const uint16_t *>(image);
    uint16_t *dstY = reinterpret_cast<uint16_t *>(buffer.data());
#ifdef __ARM_NEON
    int i = 0;
    for (; i + 8 <= ySamples; i += 8) {
        uint16x8_t y = vld1q_u16(srcY + i);
        vst1q_u16(dstY + i, vshlq_n_u16(y, 6));
    }
    for (; i < ySamples; ++i)
        dstY[i] = srcY[i] << 6;
#elif defined(__x86_64__) || defined(_M_AMD64)
    if (Util::cpuHasAVX2())
        shiftYPlane_AVX2(srcY, dstY, ySamples);
    else
        shiftYPlane_SSE2(srcY, dstY, ySamples);
#else
    for (int i = 0; i < ySamples; ++i)
        dstY[i] = srcY[i] << 6;
#endif
    const uint16_t *srcU = reinterpret_cast<const uint16_t *>(image + yPlaneSize);
    const uint16_t *srcV = reinterpret_cast<const uint16_t *>(image + yPlaneSize + uvPlaneSize);
    uint16_t *dstUV = reinterpret_cast<uint16_t *>(buffer.data() + yPlaneSize);
    const int uvSamples = uvW * uvH;
#ifdef __ARM_NEON
    int j = 0;
    for (; j + 8 <= uvSamples; j += 8) {
        uint16x8_t u = vshlq_n_u16(vld1q_u16(srcU + j), 6);
        uint16x8_t v = vshlq_n_u16(vld1q_u16(srcV + j), 6);
        uint16x8x2_t uv = {u, v};
        vst2q_u16(dstUV + j * 2, uv);
    }
    for (; j < uvSamples; ++j) {
        dstUV[2 * j] = srcU[j] << 6;
        dstUV[2 * j + 1] = srcV[j] << 6;
    }
#elif defined(__x86_64__) || defined(_M_AMD64)
    if (Util::cpuHasAVX2())
        interleaveUVPlanes_AVX2(srcU, srcV, dstUV, uvSamples);
    else
        interleaveUVPlanes_SSE2(srcU, srcV, dstUV, uvSamples);
#else
    for (int i = 0; i < uvSamples; ++i) {
        dstUV[2 * i] = srcU[i] << 6;
        dstUV[2 * i + 1] = srcV[i] << 6;
    }
#endif
}

void VideoWidget::pushFrameToSink(const SharedFrame &frame, QByteArray p016Buffer)
{
    if (!m_videoSink)
        return;
    int width = frame.get_image_width();
    int height = frame.get_image_height();
    if (width < 1 || height < 1)
        return;

    bool is10bit = !qstrcmp(m_consumer->get("mlt_image_format"), "yuv420p10");
    if (!is10bit) {
        // Validate 8-bit image is available (caches it for later use in map()).
        if (!frame.get_image(mlt_image_yuv420p))
            return;
    } else if (p016Buffer.isEmpty()) {
        // Fallback conversion on the calling thread — only reached from setVideoSink(),
        // not the normal playback path where on_frame_show() pre-converts on the MLT thread.
        const uint8_t *image = frame.get_image(mlt_image_yuv420p10);
        if (!image)
            return;
        convertToP016(image, width, height, p016Buffer);
    }

    auto pixFmt = is10bit ? QVideoFrameFormat::Format_P016 : QVideoFrameFormat::Format_YUV420P;
    QVideoFrameFormat fmt(QSize(width, height), pixFmt);
    fmt.setColorRange(QVideoFrameFormat::ColorRange_Video);

    // Determine the HDR transfer from the consumer's color_trc property, which
    // reconfigure() sets correctly regardless of whether the profile colorspace
    // is 2020 (MLT convention) or 9 (FFmpeg AVCOL_SPC_BT2020_NCL).  Checking
    // color_trc first avoids the bug where case 2020: is never reached in
    // automatic video mode and the frame is incorrectly stamped BT.709.
    const char *activeTrc = m_consumer->get("color_trc");
    const bool isHlg = !qstrcmp(activeTrc, "arib-std-b67");
    const bool isPq = !qstrcmp(activeTrc, "smpte2084");

    if (isHlg || isPq) {
        fmt.setColorSpace(QVideoFrameFormat::ColorSpace_BT2020);
        if (isHlg) {
            fmt.setColorTransfer(QVideoFrameFormat::ColorTransfer_STD_B67);
            // Use user-overridden content peak, or default to 1000 nits.
            const float hlgMaxNits = Settings.playerHdrContentPeakNits() > 0
                                         ? static_cast<float>(Settings.playerHdrContentPeakNits())
                                         : 1000.0f;
            fmt.setMaxLuminance(hlgMaxNits);
        } else {
            fmt.setColorTransfer(QVideoFrameFormat::ColorTransfer_ST2084);
            // For PQ, maxLuminance drives Qt's BT.2390 tone-mapping EETF.
            // When tone mapping is disabled, clamp at the display peak so Qt
            // applies no compression. Otherwise use the user's content-peak
            // setting (0 = auto → 1000 nits as a sensible default).
            float pqMaxNits;
            if (!Settings.playerHdrToneMapping()) {
                const int displayPeak = Settings.playerHdrDisplayPeakNits();
                pqMaxNits = displayPeak > 0 ? static_cast<float>(displayPeak) : 1000.0f;
            } else if (Settings.playerHdrContentPeakNits() > 0) {
                pqMaxNits = static_cast<float>(Settings.playerHdrContentPeakNits());
            } else {
                pqMaxNits = 1000.0f;
            }
            fmt.setMaxLuminance(pqMaxNits);
        }
        // Log whenever the stamped TRC or maxLuminance changes.
        static QByteArray s_lastTrc;
        static float s_lastMaxLum = -1.0f;
        const float stamped = fmt.maxLuminance();
        if (s_lastTrc != activeTrc || !qFuzzyCompare(s_lastMaxLum, stamped)) {
            s_lastTrc = activeTrc;
            s_lastMaxLum = stamped;
            qDebug() << "HDR pushFrameToSink: colorspace =" << profile().colorspace()
                     << "color_trc =" << activeTrc << "maxLuminance =" << stamped
                     << "toneMapping =" << Settings.playerHdrToneMapping()
                     << "contentPeakNits =" << Settings.playerHdrContentPeakNits()
                     << "displayPeakNits =" << Settings.playerHdrDisplayPeakNits();
        }
    } else {
        switch (profile().colorspace()) {
        case 601:
        case 170:
            fmt.setColorSpace(QVideoFrameFormat::ColorSpace_BT601);
            fmt.setColorTransfer(QVideoFrameFormat::ColorTransfer_BT601);
            break;
        case 2020:
            fmt.setColorSpace(QVideoFrameFormat::ColorSpace_BT2020);
            fmt.setColorTransfer(QVideoFrameFormat::ColorTransfer_BT709);
            break;
        default:
            fmt.setColorSpace(QVideoFrameFormat::ColorSpace_BT709);
            fmt.setColorTransfer(QVideoFrameFormat::ColorTransfer_BT709);
            break;
        }
    }

    // Zero-copy buffer for 8-bit, or P016 pre-converted buffer for 10-bit.
    class SharedFrameVideoBuffer : public QAbstractVideoBuffer
    {
    public:
        SharedFrameVideoBuffer(const SharedFrame &sf, const QVideoFrameFormat &f)
            : m_sharedFrame(sf)
            , m_format(f)
        {}
        SharedFrameVideoBuffer(QByteArray p016,
                               const QVideoFrameFormat &f,
                               std::function<void(QByteArray)> onFree)
            : m_p016(std::move(p016))
            , m_format(f)
            , m_onFree(std::move(onFree))
        {}
        ~SharedFrameVideoBuffer()
        {
            if (m_onFree && !m_p016.isEmpty())
                m_onFree(std::move(m_p016));
        }
        QVideoFrameFormat format() const override { return m_format; }
        MapData map(QVideoFrame::MapMode) override
        {
            int w = m_sharedFrame.get_image_width();
            int h = m_sharedFrame.get_image_height();
            MapData md;
            if (!m_p016.isEmpty()) {
                // P016: semi-planar 16-bit, 2 planes
                w = m_format.frameWidth();
                h = m_format.frameHeight();
                auto *p = reinterpret_cast<uint8_t *>(m_p016.data());
                md.planeCount = 2;
                // Y plane (16-bit per sample)
                md.bytesPerLine[0] = w * 2;
                md.data[0] = p;
                md.dataSize[0] = w * h * 2;
                // Interleaved UV plane (2 × 16-bit per sample pair)
                md.bytesPerLine[1] = (w / 2) * 4;
                md.data[1] = p + md.dataSize[0];
                md.dataSize[1] = (w / 2) * (h / 2) * 4;
            } else {
                // YUV420P: planar 8-bit, 3 planes
                auto *p = const_cast<uint8_t *>(m_sharedFrame.get_image(mlt_image_yuv420p));
                md.planeCount = 3;
                md.bytesPerLine[0] = w;
                md.data[0] = p;
                md.dataSize[0] = w * h;
                md.bytesPerLine[1] = w / 2;
                md.data[1] = p + md.dataSize[0];
                md.dataSize[1] = (w / 2) * (h / 2);
                md.bytesPerLine[2] = w / 2;
                md.data[2] = md.data[1] + md.dataSize[1];
                md.dataSize[2] = md.dataSize[1];
            }
            return md;
        }

    private:
        SharedFrame m_sharedFrame;
        QByteArray m_p016;
        QVideoFrameFormat m_format;
        std::function<void(QByteArray)> m_onFree;
    };

    std::unique_ptr<SharedFrameVideoBuffer> buffer;
    if (is10bit) {
        buffer = std::make_unique<SharedFrameVideoBuffer>(std::move(p016Buffer),
                                                          fmt,
                                                          [pool = std::weak_ptr<P016Pool>(
                                                               m_p016Pool)](QByteArray buf) {
                                                              if (auto p = pool.lock()) {
                                                                  QMutexLocker lock(&p->mutex);
                                                                  if (p->buffers.size() < 3)
                                                                      p->buffers.append(
                                                                          std::move(buf));
                                                              }
                                                          });
    } else {
        buffer = std::make_unique<SharedFrameVideoBuffer>(frame, fmt);
    }
    QVideoFrame videoFrame(std::move(buffer));
    // Set PTS so downstream consumers (e.g. HdrPreviewWindow) can track position.
    const double fps = profile().fps();
    if (fps > 0.0)
        videoFrame.setStartTime(qRound64(frame.get_position() / fps * 1000000.0));
    m_videoSink->setVideoFrame(videoFrame);
    emit videoFrameReady(videoFrame);
}

void VideoWidget::showFrame(Mlt::Frame frame, QByteArray p016Buffer)
{
    m_mutex.lock();
    m_sharedFrame = SharedFrame(frame);
    m_mutex.unlock();
    bool isVui = m_sharedFrame.get_int(kShotcutVuiMetaProperty) && !m_hideVui;
    if (!isVui && source() != QmlUtilities::blankVui()) {
        m_savedQmlSource = source();
        setSource(QmlUtilities::blankVui());
    } else if (isVui && !m_savedQmlSource.isEmpty() && source() != m_savedQmlSource) {
        setSource(m_savedQmlSource);
    }
    pushFrameToSink(m_sharedFrame, std::move(p016Buffer));
    emit frameDisplayed(m_sharedFrame);
    if (m_imageRequested) {
        m_imageRequested = false;
        emit imageReady();
    }
    m_frameSemaphore.release();
}

void VideoWidget::setGrid(int grid)
{
    m_grid = grid;
    emit gridChanged();
    quickWindow()->update();
}

void VideoWidget::setZoom(float zoom)
{
    m_zoom = zoom;
    emit zoomChanged();
    // Reset the VUI control
    setSource(source());
    quickWindow()->update();
}

void VideoWidget::setOffsetX(int x)
{
    m_offset.setX(x);
    emit offsetChanged();
    quickWindow()->update();
}

void VideoWidget::setOffsetY(int y)
{
    m_offset.setY(y);
    emit offsetChanged();
    quickWindow()->update();
}

void VideoWidget::setCurrentFilter(QmlFilter *filter, QmlMetadata *meta)
{
    m_hideVui = false;
    if (meta && meta->type() == QmlMetadata::Filter
        && QFile::exists(meta->vuiFilePath().toLocalFile())) {
        filter->producer().set(kShotcutVuiMetaProperty, 1);
        rootContext()->setContextProperty("filter", filter);
        setSource(meta->vuiFilePath());
        refreshConsumer();
    } else {
        setBlankScene();
    }
}

void VideoWidget::setSnapToGrid(bool snap)
{
    m_snapToGrid = snap;
    emit snapToGridChanged();
}

// MLT consumer-frame-show event handler — runs on the MLT consumer thread.
// P016 conversion for 10-bit frames is done here so the GUI thread is not burdened.
void VideoWidget::on_frame_show(mlt_consumer, VideoWidget *widget, mlt_event_data data)
{
    auto frame = Mlt::EventData(data).to_frame();
    if (frame.is_valid() && frame.get_int("rendered")) {
        int timeout = (widget->consumer()->get_int("real_time") > 0) ? 0 : 1000;
        if (widget->m_frameRenderer) {
            // Old path: dispatch to FrameRenderer thread
            if (widget->m_frameRenderer->semaphore()->tryAcquire(1, timeout)) {
                QMetaObject::invokeMethod(widget->m_frameRenderer,
                                          "showFrame",
                                          Qt::QueuedConnection,
                                          Q_ARG(Mlt::Frame, frame));
            } else if (!Settings.playerRealtime()) {
                LOG_WARNING() << "VideoWidget dropped frame" << frame.get_position();
            }
        } else {
            // New QVideoSink path: pre-convert P016 on this thread then hand off to GUI
            if (widget->m_frameSemaphore.tryAcquire(1, timeout)) {
                QByteArray p016Buffer;
                if (!qstrcmp(widget->consumer()->get("mlt_image_format"), "yuv420p10")) {
                    mlt_image_format mltFmt = mlt_image_yuv420p10;
                    int width = 0, height = 0;
                    const uint8_t *image = frame.get_image(mltFmt, width, height);
                    if (image && width > 0 && height > 0) {
                        // Grab a reusable buffer from the pool to avoid per-frame allocation.
                        {
                            QMutexLocker lock(&widget->m_p016Pool->mutex);
                            if (!widget->m_p016Pool->buffers.isEmpty()) {
                                p016Buffer = std::move(widget->m_p016Pool->buffers.last());
                                widget->m_p016Pool->buffers.removeLast();
                            }
                        }
                        convertToP016(image, width, height, p016Buffer);
                    }
                }
                QMetaObject::invokeMethod(
                    widget,
                    [widget, frame, buf = std::move(p016Buffer)]() mutable {
                        widget->showFrame(frame, std::move(buf));
                    },
                    Qt::QueuedConnection);
            } else if (!Settings.playerRealtime()) {
                LOG_WARNING() << "VideoWidget dropped frame" << frame.get_position();
            }
        }
    }
}

RenderThread::RenderThread(thread_function_t function, void *data)
    : QThread{nullptr}
    , m_function{function}
    , m_data{data}
    , m_context{new QOpenGLContext}
    , m_surface{new QOffscreenSurface}
{
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setMajorVersion(3);
    format.setMinorVersion(2);
    format.setDepthBufferSize(0);
    format.setStencilBufferSize(0);
    m_context->setFormat(format);
    m_context->create();
    m_context->moveToThread(this);
    m_surface->setFormat(format);
    m_surface->create();
}

RenderThread::~RenderThread()
{
    m_surface->destroy();
}

void RenderThread::run()
{
    Q_ASSERT(m_context->isValid());
    m_context->makeCurrent(m_surface.get());
    m_function(m_data);
    m_context->doneCurrent();
}

FrameRenderer::FrameRenderer()
    : QThread(nullptr)
    , m_semaphore(3)
    , m_imageRequested(false)
{
    setObjectName("FrameRenderer");
    moveToThread(this);
    start();
}

FrameRenderer::~FrameRenderer() {}

void FrameRenderer::showFrame(Mlt::Frame frame)
{
    SharedFrame newFrame(frame);
    bool emitImage = false;
    {
        QMutexLocker locker(&m_mutex);
        m_displayFrame = newFrame;
        if (m_imageRequested) {
            m_imageRequested = false;
            emitImage = true;
        }
    }
    emit frameDisplayed(newFrame);
    if (emitImage)
        emit imageReady();
    m_semaphore.release();
}

void FrameRenderer::requestImage()
{
    QMutexLocker locker(&m_mutex);
    m_imageRequested = true;
}

SharedFrame FrameRenderer::getDisplayFrame()
{
    QMutexLocker locker(&m_mutex);
    return m_displayFrame;
}
