/*7
 * Copyright (c) 2011-2023 Meltytech, LLC
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

#include <QtWidgets>
#include <QUrl>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QtQml>
#include <QQuickItem>
#include <Mlt.h>
#include <Logger.h>
#include "videowidget.h"
#include "settings.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlfilter.h"
#include "mainwindow.h"

using namespace Mlt;

VideoWidget::VideoWidget(QObject *parent)
    : QQuickWidget(QmlUtilities::sharedEngine(), (QWidget *) parent)
    , Controller()
    , m_grid(0)
    , m_initSem(0)
    , m_isInitialized(false)
    , m_frameRenderer(nullptr)
    , m_zoom(0.0f)
    , m_offset(QPoint(0, 0))
    , m_snapToGrid(true)
    , m_scrubAudio(false)
    , m_maxTextureSize(4096)
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

    connect(quickWindow(), &QQuickWindow::visibilityChanged, this, &VideoWidget::setBlankScene,
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
    m_frameRenderer = new FrameRenderer();
    connect(m_frameRenderer, &FrameRenderer::frameDisplayed, this,
            &VideoWidget::onFrameDisplayed, Qt::QueuedConnection);
    connect(m_frameRenderer, &FrameRenderer::frameDisplayed, this,
            &VideoWidget::frameDisplayed, Qt::QueuedConnection);
    connect(m_frameRenderer, SIGNAL(imageReady()), SIGNAL(imageReady()));
    m_initSem.release();
    m_isInitialized = true;
    LOG_DEBUG() << "end";
}

void VideoWidget::renderVideo()
{
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
    if (event->isAccepted()) return;
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
    if (event->isAccepted()) return;
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

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(Mlt::XmlMimeType, MLT.XML().toUtf8());
    drag->setMimeData(mimeData);
    mimeData->setText(QString::number(MLT.producer()->get_playtime()));
    if (m_frameRenderer && m_frameRenderer->getDisplayFrame().is_valid()) {
        Mlt::Frame displayFrame(m_frameRenderer->getDisplayFrame().clone(false, true));
        QImage displayImage = MLT.image(&displayFrame, 45 * MLT.profile().dar(), 45).scaledToHeight(45);
        drag->setPixmap(QPixmap::fromImage(displayImage));
    }
    drag->setHotSpot(QPoint(0, 0));
    drag->exec(Qt::LinkAction);
}

void VideoWidget::keyPressEvent(QKeyEvent *event)
{
    QQuickWidget::keyPressEvent(event);
    if (event->isAccepted()) return;
    MAIN.keyPressEvent(event);
}

bool VideoWidget::event(QEvent *event)
{
    bool result = QQuickWidget::event(event);
    if (event->type() == QEvent::PaletteChange && m_sharedFrame.is_valid())
        onFrameDisplayed(m_sharedFrame);
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
    (*thread) = new RenderThread(function, data);
    (*thread)->start();
}

static void onThreadCreate(mlt_properties owner, VideoWidget *self, mlt_event_data data)
{
    Q_UNUSED(owner)
    auto threadData = (mlt_event_data_thread *) Mlt::EventData(data).to_object();
    if (threadData) {
        self->createThread((RenderThread **) threadData->thread, threadData->function, threadData->data);
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
            delete renderThread;
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
            m_consumer.reset(new Mlt::FilteredConsumer(previewProfile(), serviceName.toLatin1().constData()));

        m_threadStartEvent.reset();
        m_threadStopEvent.reset();
        m_threadCreateEvent.reset();
        m_threadJoinEvent.reset();
    }
    if (m_consumer->is_valid()) {
        // Connect the producer to the consumer - tell it to "run" later
        m_consumer->connect(*m_producer);
        // Make an event handler for when a frame's image should be displayed
        m_consumer->listen("consumer-frame-show", this, (mlt_listener) on_frame_show);
        m_consumer->set("real_time", MLT.realTime());
        m_consumer->set("mlt_image_format", m_glslManager ? "rgba" : "yuv422");
        m_consumer->set("channels", property("audio_channels").toInt());

        if (isMulti) {
            m_consumer->set("terminate_on_pause", 0);
            m_consumer->set("0", serviceName.toLatin1().constData());
            if (!profile().progressive())
                m_consumer->set("0.progressive", property("progressive").toBool());
            m_consumer->set("0.rescale", property("rescale").toString().toLatin1().constData());
            m_consumer->set("0.deinterlace_method",
                            property("deinterlace_method").toString().toLatin1().constData());
            m_consumer->set("0.buffer", qMax(25, qRound(profile().fps())));
            m_consumer->set("0.prefill", qMax(1, qRound(profile().fps() / 25.0)));
            m_consumer->set("0.drop_max", qRound(profile().fps() / 4.0));
            if (property("keyer").isValid())
                m_consumer->set("0.keyer", property("keyer").toInt());
            m_consumer->set("0.video_delay", Settings.playerVideoDelayMs());
        } else {
            if (!profile().progressive())
                m_consumer->set("progressive", property("progressive").toBool());
            m_consumer->set("rescale", property("rescale").toString().toLatin1().constData());
            m_consumer->set("deinterlace_method",
                            property("deinterlace_method").toString().toLatin1().constData());
            m_consumer->set("buffer", qMax(25, qRound(profile().fps())));
            m_consumer->set("prefill", qMax(1, qRound(profile().fps() / 25.0)));
            m_consumer->set("drop_max", qRound(profile().fps() / 4.0));
            if (property("keyer").isValid())
                m_consumer->set("keyer", property("keyer").toInt());
            m_consumer->set("video_delay", Settings.playerVideoDelayMs());
        }
        if (m_glslManager) {
            if (!m_threadCreateEvent)
                m_threadCreateEvent.reset(m_consumer->listen("consumer-thread-create", this,
                                                             (mlt_listener) onThreadCreate));
            if (!m_threadJoinEvent)
                m_threadJoinEvent.reset(m_consumer->listen("consumer-thread-join", this,
                                                           (mlt_listener) onThreadJoin));
            if (!m_threadStartEvent)
                m_threadStartEvent.reset(m_consumer->listen("consumer-thread-started", this,
                                                            (mlt_listener) onThreadStarted));
            if (!m_threadStopEvent)
                m_threadStopEvent.reset(m_consumer->listen("consumer-thread-stopped", this,
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
        return QPoint(m_offset.x() - (MLT.profile().width()  * m_zoom -  width()) / 2,
                      m_offset.y() - (MLT.profile().height() * m_zoom - height()) / 2);
    }
}

QImage VideoWidget::image() const
{
    SharedFrame frame = m_frameRenderer->getDisplayFrame();
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
    SharedFrame frame = m_frameRenderer->getDisplayFrame();
    if (frame.is_valid()) {
        Mlt::Producer *frameProducer = frame.get_original_producer();
        if (frameProducer && frameProducer->is_valid() && frameProducer->get_int(kIsProxyProperty)) {
            isProxy = true;
        }
        delete frameProducer;
    }
    return isProxy;
}

void VideoWidget::requestImage() const
{
    m_frameRenderer->requestImage();
}

void VideoWidget::onFrameDisplayed(const SharedFrame &frame)
{
    m_mutex.lock();
    m_sharedFrame = frame;
    m_mutex.unlock();
    bool isVui = frame.get_int(kShotcutVuiMetaProperty);
    if (!isVui && source() != QmlUtilities::blankVui()) {
        m_savedQmlSource = source();
        setSource(QmlUtilities::blankVui());
    } else if (isVui && !m_savedQmlSource.isEmpty() && source() != m_savedQmlSource) {
        setSource(m_savedQmlSource);
    }
    quickWindow()->update();
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

// MLT consumer-frame-show event handler
void VideoWidget::on_frame_show(mlt_consumer, VideoWidget *widget, mlt_event_data data)
{
    auto frame = Mlt::EventData(data).to_frame();
    if (frame.is_valid() && frame.get_int("rendered")) {
        int timeout = (widget->consumer()->get_int("real_time") > 0) ? 0 : 1000;
        if (widget->m_frameRenderer && widget->m_frameRenderer->semaphore()->tryAcquire(1, timeout)) {
            QMetaObject::invokeMethod(widget->m_frameRenderer, "showFrame", Qt::QueuedConnection,
                                      Q_ARG(Mlt::Frame, frame));
        } else if (!Settings.playerRealtime()) {
            LOG_WARNING() << "VideoWidget dropped frame" << frame.get_position();
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

FrameRenderer::~FrameRenderer()
{
}

void FrameRenderer::showFrame(Mlt::Frame frame)
{
    m_displayFrame = SharedFrame(frame);
    emit frameDisplayed(m_displayFrame);

    if (m_imageRequested) {
        m_imageRequested = false;
        emit imageReady();
    }

    m_semaphore.release();
}

void FrameRenderer::requestImage()
{
    m_imageRequested = true;
}

SharedFrame FrameRenderer::getDisplayFrame()
{
    return m_displayFrame;
}
