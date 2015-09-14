/*
 * Copyright (c) 2011-2015 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * GL shader based on BSD licensed code from Peter Bengtsson:
 * http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
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
#include <QOpenGLFunctions_3_2_Core>
#include <QUrl>
#include <QOffscreenSurface>
#include <QtQml>
#include <Mlt.h>
#include "glwidget.h"
#include "settings.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlfilter.h"
#include "mainwindow.h"

#define USE_GL_SYNC // Use glFinish() if not defined.

#ifdef QT_NO_DEBUG
#define check_error(fn) {}
#else
#define check_error(fn) { int err = fn->glGetError(); if (err != GL_NO_ERROR) { qCritical() << "GL error"  << hex << err << dec << "at" << __FILE__ << ":" << __LINE__; } }
#endif

#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFFull
#endif

#ifndef Q_OS_WIN
typedef GLenum (*ClientWaitSync_fp) (GLsync sync, GLbitfield flags, GLuint64 timeout);
static ClientWaitSync_fp ClientWaitSync = 0;
#endif

using namespace Mlt;

GLWidget::GLWidget(QObject *parent)
    : QQuickView(QmlUtilities::sharedEngine(), (QWindow*) parent)
    , Controller()
    , m_shader(0)
    , m_glslManager(0)
    , m_initSem(0)
    , m_isInitialized(false)
    , m_threadStartEvent(0)
    , m_threadStopEvent(0)
    , m_threadCreateEvent(0)
    , m_threadJoinEvent(0)
    , m_frameRenderer(0)
    , m_zoom(0.0f)
    , m_offset(QPoint(0, 0))
    , m_shareContext(0)
{
    qDebug() << "begin";
    m_texture[0] = m_texture[1] = m_texture[2] = 0;
    setPersistentOpenGLContext(true);
    setPersistentSceneGraph(true);
    setClearBeforeRendering(false);
    setResizeMode(QQuickView::SizeRootObjectToView);
    QDir importPath = QmlUtilities::qmlDir();
    importPath.cd("modules");
    engine()->addImportPath(importPath.path());
    QmlUtilities::setCommonProperties(rootContext());
    rootContext()->setContextProperty("video", this);

    if (Settings.playerGPU())
        m_glslManager = new Filter(profile(), "glsl.manager");
    if ((m_glslManager && !m_glslManager->is_valid())) {
        delete m_glslManager;
        m_glslManager = 0;
    }

    connect(this, SIGNAL(openglContextCreated(QOpenGLContext*)), SLOT(onOpenGLContextCreated(QOpenGLContext*)), Qt::DirectConnection);
    connect(this, SIGNAL(sceneGraphInitialized()), SLOT(initializeGL()), Qt::DirectConnection);
    connect(this, SIGNAL(sceneGraphInitialized()), SLOT(setBlankScene()), Qt::QueuedConnection);
    connect(this, SIGNAL(beforeRendering()), SLOT(paintGL()), Qt::DirectConnection);
    qDebug() << "end";
}

GLWidget::~GLWidget()
{
    qDebug();
    stop();
    delete m_glslManager;
    delete m_threadStartEvent;
    delete m_threadStopEvent;
    delete m_threadCreateEvent;
    delete m_threadJoinEvent;
    if (m_frameRenderer && m_frameRenderer->isRunning()) {
        QMetaObject::invokeMethod(m_frameRenderer, "cleanup");
        m_frameRenderer->quit();
        m_frameRenderer->wait();
        m_frameRenderer->deleteLater();
    }
    delete m_shareContext;
    delete m_shader;
}

void GLWidget::initializeGL()
{
    qDebug() << "begin";
    if (m_isInitialized) return;

    initializeOpenGLFunctions();
    qDebug() << "OpenGL vendor" << QString::fromUtf8((const char*) glGetString(GL_VENDOR));
    qDebug() << "OpenGL renderer" << QString::fromUtf8((const char*) glGetString(GL_RENDERER));
    qDebug() << "OpenGL threaded?" << openglContext()->supportsThreadedOpenGL();
    qDebug() << "OpenGL ES?" << openglContext()->isOpenGLES();

    if (m_glslManager && openglContext()->isOpenGLES()) {
        delete m_glslManager;
        m_glslManager = 0;
        // Need to destroy MLT global reference to prevent filters from trying to use GPU.
        mlt_properties_set_data(mlt_global_properties(), "glslManager", NULL, 0, NULL, NULL);
        emit gpuNotSupported();
    }

    createShader();

#if defined(USE_GL_SYNC) && !defined(Q_OS_WIN)
    // getProcAddress is not working for me on Windows.
    if (Settings.playerGPU()) {
        if (m_glslManager && openglContext()->hasExtension("GL_ARB_sync")) {
            ClientWaitSync = (ClientWaitSync_fp) openglContext()->getProcAddress("glClientWaitSync");
        }
        if (!ClientWaitSync) {
            emit gpuNotSupported();
            delete m_glslManager;
            m_glslManager = 0;
        }
    }
#endif

    openglContext()->doneCurrent();
    if (m_glslManager) {
        // Create a context sharing with this context for the RenderThread context.
        // This is needed because openglContext() is active in another thread
        // at the time that RenderThread is created.
        // See this Qt bug for more info: https://bugreports.qt.io/browse/QTBUG-44677
        m_shareContext = new QOpenGLContext;
        m_shareContext->setFormat(openglContext()->format());
        m_shareContext->setShareContext(openglContext());
        m_shareContext->create();
    }
    m_frameRenderer = new FrameRenderer(openglContext(), &m_offscreenSurface);
    openglContext()->makeCurrent(this);

    connect(m_frameRenderer, SIGNAL(frameDisplayed(const SharedFrame&)), this, SIGNAL(frameDisplayed(const SharedFrame&)), Qt::QueuedConnection);
    if (openglContext()->supportsThreadedOpenGL())
        connect(m_frameRenderer, SIGNAL(textureReady(GLuint,GLuint,GLuint)), SLOT(updateTexture(GLuint,GLuint,GLuint)), Qt::DirectConnection);
    else
        connect(m_frameRenderer, SIGNAL(frameDisplayed(const SharedFrame&)), SLOT(onFrameDisplayed(const SharedFrame&)), Qt::QueuedConnection);
    connect(this, SIGNAL(textureUpdated()), SLOT(update()), Qt::QueuedConnection);

    m_initSem.release();
    m_isInitialized = true;
    qDebug() << "end";
}

void GLWidget::setBlankScene()
{
    setSource(QmlUtilities::blankVui());
}

void GLWidget::resizeGL(int width, int height)
{
    int x, y, w, h;
    double this_aspect = (double) width / height;
    double video_aspect = profile().dar();

    // Special case optimisation to negate odd effect of sample aspect ratio
    // not corresponding exactly with image resolution.
    if ((int) (this_aspect * 1000) == (int) (video_aspect * 1000))
    {
        w = width;
        h = height;
    }
    // Use OpenGL to normalise sample aspect ratio
    else if (height * video_aspect > width)
    {
        w = width;
        h = width / video_aspect;
    }
    else
    {
        w = height * video_aspect;
        h = height;
    }
    x = (width - w) / 2;
    y = (height - h) / 2;
    m_rect.setRect(x, y, w, h);
    emit rectChanged();
}

void GLWidget::resizeEvent(QResizeEvent* event)
{
    QQuickView::resizeEvent(event);
    resizeGL(event->size().width(), event->size().height());
}

void GLWidget::createShader()
{
    m_shader = new QOpenGLShaderProgram;
    m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                     "uniform highp mat4 projection;"
                                     "uniform highp mat4 modelView;"
                                     "attribute highp vec4 vertex;"
                                     "attribute highp vec2 texCoord;"
                                     "varying highp vec2 coordinates;"
                                     "void main(void) {"
                                     "  gl_Position = projection * modelView * vertex;"
                                     "  coordinates = texCoord;"
                                     "}");
    if (m_glslManager) {
        m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                          "uniform sampler2D tex;"
                                          "varying highp vec2 coordinates;"
                                          "void main(void) {"
                                          "  gl_FragColor = texture2D(tex, coordinates);"
                                          "}");
        m_shader->link();
        m_textureLocation[0] = m_shader->uniformLocation("tex");
    } else {
        m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                          "uniform sampler2D Ytex, Utex, Vtex;"
                                          "uniform lowp int colorspace;"
                                          "varying highp vec2 coordinates;"
                                          "void main(void) {"
                                          "  mediump vec3 texel;"
                                          "  texel.r = texture2D(Ytex, coordinates).r - 0.0625;" // Y
                                          "  texel.g = texture2D(Utex, coordinates).r - 0.5;"    // U
                                          "  texel.b = texture2D(Vtex, coordinates).r - 0.5;"    // V
                                          "  mediump mat3 coefficients;"
                                          "  if (colorspace == 601) {"
                                          "    coefficients = mat3("
                                          "      1.1643,  1.1643,  1.1643," // column 1
                                          "      0.0,    -0.39173, 2.017," // column 2
                                          "      1.5958, -0.8129,  0.0);" // column 3
                                          "  } else {" // ITU-R 709
                                          "    coefficients = mat3("
                                          "      1.1643, 1.1643, 1.1643," // column 1
                                          "      0.0,   -0.213,  2.112," // column 2
                                          "      1.793, -0.533,  0.0);" // column 3
                                          "  }"
                                          "  gl_FragColor = vec4(coefficients * texel, 1.0);"
                                          "}");
        m_shader->link();
        m_textureLocation[0] = m_shader->uniformLocation("Ytex");
        m_textureLocation[1] = m_shader->uniformLocation("Utex");
        m_textureLocation[2] = m_shader->uniformLocation("Vtex");
        m_colorspaceLocation = m_shader->uniformLocation("colorspace");
    }
    m_projectionLocation = m_shader->uniformLocation("projection");
    m_modelViewLocation = m_shader->uniformLocation("modelView");
    m_vertexLocation = m_shader->attributeLocation("vertex");
    m_texCoordLocation = m_shader->attributeLocation("texCoord");
}

static void uploadTextures(QOpenGLContext* context, SharedFrame& frame, GLuint texture[])
{
    int width = frame.get_image_width();
    int height = frame.get_image_height();
    const uint8_t* image = frame.get_image();
    QOpenGLFunctions* f = context->functions();

    // Upload each plane of YUV to a texture.
    if (texture[0])
        f->glDeleteTextures(3, texture);
    check_error(f);
    f->glGenTextures(3, texture);
    check_error(f);

    f->glBindTexture  (GL_TEXTURE_2D, texture[0]);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    check_error(f);
    f->glTexImage2D   (GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, image);
    check_error(f);

    f->glBindTexture  (GL_TEXTURE_2D, texture[1]);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    check_error(f);
    f->glTexImage2D   (GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2, 0,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, image + width * height);
    check_error(f);

    f->glBindTexture  (GL_TEXTURE_2D, texture[2]);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    check_error(f);
    f->glTexImage2D   (GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2, 0,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, image + width * height + width/2 * height/2);
    check_error(f);
}

void GLWidget::paintGL()
{
    QOpenGLFunctions* f = openglContext()->functions();
    int width = this->width() * devicePixelRatio();
    int height = this->height() * devicePixelRatio();

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glViewport(0, 0, width, height);
    check_error(f);
    QColor color = QPalette().color(QPalette::Window);
    glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glClear(GL_COLOR_BUFFER_BIT);
    check_error(f);

    if (!openglContext()->supportsThreadedOpenGL()) {
        m_mutex.lock();
        if (!m_sharedFrame.is_valid()) {
            m_mutex.unlock();
            return;
        }
        uploadTextures(openglContext(), m_sharedFrame, m_texture);
        m_mutex.unlock();
    }

    if (!m_texture[0]) return;

    // Bind textures.
    for (int i = 0; i < 3; ++i) {
        if (m_texture[i]) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, m_texture[i]);
            check_error(f);
        }
    }

    // Init shader program.
    m_shader->bind();
    if (m_glslManager) {
        m_shader->setUniformValue(m_textureLocation[0], 0);
    } else {
        m_shader->setUniformValue(m_textureLocation[0], 0);
        m_shader->setUniformValue(m_textureLocation[1], 1);
        m_shader->setUniformValue(m_textureLocation[2], 2);
        m_shader->setUniformValue(m_colorspaceLocation, MLT.profile().colorspace());
    }
    check_error(f);

    // Setup an orthographic projection.
    QMatrix4x4 projection;
    projection.scale(2.0f / width, 2.0f / height);
    m_shader->setUniformValue(m_projectionLocation, projection);
    check_error(f);

    // Set model view.
    QMatrix4x4 modelView;
    if (m_zoom > 0.0) {
        if (offset().x() || offset().y())
            modelView.translate(-offset().x() * devicePixelRatio(),
                                 offset().y() * devicePixelRatio());
        modelView.scale(zoom(), zoom());
    }
    m_shader->setUniformValue(m_modelViewLocation, modelView);
    check_error(f);

    // Provide vertices of triangle strip.
    QVector<QVector2D> vertices;
    width = m_rect.width() * devicePixelRatio();
    height = m_rect.height() * devicePixelRatio();
    vertices << QVector2D(float(-width)/2.0f, float(-height)/2.0f);
    vertices << QVector2D(float(-width)/2.0f, float( height)/2.0f);
    vertices << QVector2D(float( width)/2.0f, float(-height)/2.0f);
    vertices << QVector2D(float( width)/2.0f, float( height)/2.0f);
    m_shader->enableAttributeArray(m_vertexLocation);
    check_error(f);
    m_shader->setAttributeArray(m_vertexLocation, vertices.constData());
    check_error(f);

    // Provide texture coordinates.
    QVector<QVector2D> texCoord;
    texCoord << QVector2D(0.0f, 1.0f);
    texCoord << QVector2D(0.0f, 0.0f);
    texCoord << QVector2D(1.0f, 1.0f);
    texCoord << QVector2D(1.0f, 0.0f);
    m_shader->enableAttributeArray(m_texCoordLocation);
    check_error(f);
    m_shader->setAttributeArray(m_texCoordLocation, texCoord.constData());
    check_error(f);

    // Render
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size());
    check_error(f);

    // Cleanup
    m_shader->disableAttributeArray(m_vertexLocation);
    m_shader->disableAttributeArray(m_texCoordLocation);
    m_shader->release();
    for (int i = 0; i < 3; ++i) {
        if (m_texture[i]) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
            check_error(f);
        }
    }
    glActiveTexture(GL_TEXTURE0);
    check_error(f);
}

void GLWidget::mousePressEvent(QMouseEvent* event)
{
    QQuickView::mousePressEvent(event);
    if (event->isAccepted()) return;
    if (event->button() == Qt::LeftButton)
        m_dragStart = event->pos();
    if (MLT.isClip())
        emit dragStarted();
}

void GLWidget::mouseMoveEvent(QMouseEvent* event)
{
    QQuickView::mouseMoveEvent(event);
    if (event->isAccepted()) return;
    if (event->modifiers() == Qt::ShiftModifier && m_producer) {
        emit seekTo(m_producer->get_length() * event->x() / width());
        return;
    }
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - m_dragStart).manhattanLength() < QApplication::startDragDistance())
        return;
    if (!MLT.producer() || !MLT.isClip())
        return;
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(Mlt::XmlMimeType, MLT.XML().toUtf8());
    drag->setMimeData(mimeData);
    mimeData->setText(QString::number(MLT.producer()->get_playtime()));
    if (m_frameRenderer && !m_glslManager && m_frameRenderer->getDisplayFrame().is_valid()) {
        Mlt::Frame displayFrame(m_frameRenderer->getDisplayFrame().clone(false, true));
        QImage displayImage = MLT.image(&displayFrame, 45 * MLT.profile().dar(), 45).scaledToHeight(45);
        drag->setPixmap(QPixmap::fromImage(displayImage));
    }
    drag->setHotSpot(QPoint(0, 0));
    drag->exec(Qt::LinkAction);
}

void GLWidget::keyPressEvent(QKeyEvent* event)
{
    QQuickView::keyPressEvent(event);
    if (event->isAccepted()) return;
    MAIN.keyPressEvent(event);
}

void GLWidget::createThread(RenderThread **thread, thread_function_t function, void *data)
{
#ifdef Q_OS_WIN
    // On Windows, MLT event consumer-thread-create is fired from the Qt main thread.
    while (!m_isInitialized)
        qApp->processEvents();
#else
    if (!m_isInitialized) {
        m_initSem.acquire();
    }
#endif
    (*thread) = new RenderThread(function, data, m_shareContext, &m_offscreenSurface);
    (*thread)->start();
}

static void onThreadCreate(mlt_properties owner, GLWidget* self,
    RenderThread** thread, int* priority, thread_function_t function, void* data )
{
    Q_UNUSED(owner)
    Q_UNUSED(priority)
    self->createThread(thread, function, data);
}

static void onThreadJoin(mlt_properties owner, GLWidget* self, RenderThread* thread)
{
    Q_UNUSED(owner)
    Q_UNUSED(self)
    if (thread) {
        thread->quit();
        thread->wait();
        delete thread;
    }
}

void GLWidget::startGlsl()
{
    if (m_glslManager) {
        m_glslManager->fire_event("init glsl");
        if (!m_glslManager->get_int("glsl_supported")) {
            delete m_glslManager;
            m_glslManager = 0;
            // Need to destroy MLT global reference to prevent filters from trying to use GPU.
            mlt_properties_set_data(mlt_global_properties(), "glslManager", NULL, 0, NULL, NULL);
            emit gpuNotSupported();
        }
        else {
            emit started();
        }
    }
}

static void onThreadStarted(mlt_properties owner, GLWidget* self)
{
    Q_UNUSED(owner)
    self->startGlsl();
}

void GLWidget::stopGlsl()
{
    //TODO This is commented out for now because it is causing crashes.
    //Technically, this should be the correct thing to do, but it appears
    //some changes in the 15.01 and 15.03 releases have created regression
    //with respect to restarting the consumer in GPU mode.
//    m_glslManager->fire_event("close glsl");
    m_texture[0] = 0;
}

static void onThreadStopped(mlt_properties owner, GLWidget* self)
{
    Q_UNUSED(owner)
    self->stopGlsl();
}

int GLWidget::setProducer(Mlt::Producer* producer, bool isMulti)
{
    int error = Controller::setProducer(producer, isMulti);

    if (!error) {
        error = reconfigure(isMulti);
        if (!error) {
            // The profile display aspect ratio may have changed.
            resizeGL(width(), height());
        }
    }
    return error;
}

int GLWidget::reconfigure(bool isMulti)
{
    int error = 0;

    // use SDL for audio, OpenGL for video
    QString serviceName = property("mlt_service").toString();
    if (!m_consumer || !m_consumer->is_valid()) {
        if (serviceName.isEmpty()) {
            m_consumer = new Mlt::FilteredConsumer(profile(), "sdl_audio");
            if (m_consumer->is_valid())
                serviceName = "sdl_audio";
            else
                serviceName = "rtaudio";
            delete m_consumer;
        }
        if (isMulti)
            m_consumer = new Mlt::FilteredConsumer(profile(), "multi");
        else
            m_consumer = new Mlt::FilteredConsumer(profile(), serviceName.toLatin1().constData());

        delete m_threadStartEvent;
        m_threadStartEvent = 0;
        delete m_threadStopEvent;
        m_threadStopEvent = 0;

        delete m_threadCreateEvent;
        m_threadCreateEvent = m_consumer->listen("consumer-thread-create", this, (mlt_listener) onThreadCreate);
        delete m_threadJoinEvent;
        m_threadJoinEvent = m_consumer->listen("consumer-thread-join", this, (mlt_listener) onThreadJoin);
    }
    if (m_consumer->is_valid()) {
        // Connect the producer to the consumer - tell it to "run" later
        m_consumer->connect(*m_producer);
        // Make an event handler for when a frame's image should be displayed
        m_consumer->listen("consumer-frame-show", this, (mlt_listener) on_frame_show);
        m_consumer->set("real_time", MLT.realTime());
        m_consumer->set("mlt_image_format", "yuv422");
        m_consumer->set("color_trc", Settings.playerGamma().toLatin1().constData());

        if (isMulti) {
            m_consumer->set("terminate_on_pause", 0);
            m_consumer->set("0", serviceName.toLatin1().constData());
            if (serviceName == "sdl_audio")
#ifdef Q_OS_WIN
                m_consumer->set("0.audio_buffer", 2048);
#else
                m_consumer->set("0.audio_buffer", 512);
#endif
            if (!profile().progressive())
                m_consumer->set("0.progressive", property("progressive").toBool());
            m_consumer->set("0.rescale", property("rescale").toString().toLatin1().constData());
            m_consumer->set("0.deinterlace_method", property("deinterlace_method").toString().toLatin1().constData());
            m_consumer->set("0.buffer", 25);
            m_consumer->set("0.prefill", 1);
            if (property("keyer").isValid())
                m_consumer->set("0.keyer", property("keyer").toInt());
        }
        else {
            if (serviceName == "sdl_audio")
#ifdef Q_OS_WIN
                m_consumer->set("audio_buffer", 2048);
#else
                m_consumer->set("audio_buffer", 512);
#endif
            if (!profile().progressive())
                m_consumer->set("progressive", property("progressive").toBool());
            m_consumer->set("rescale", property("rescale").toString().toLatin1().constData());
            m_consumer->set("deinterlace_method", property("deinterlace_method").toString().toLatin1().constData());
            m_consumer->set("buffer", 25);
            m_consumer->set("prefill", 1);
            if (property("keyer").isValid())
                m_consumer->set("keyer", property("keyer").toInt());
        }
        if (m_glslManager) {
            if (!m_threadStartEvent)
                m_threadStartEvent = m_consumer->listen("consumer-thread-started", this, (mlt_listener) onThreadStarted);
            if (!m_threadStopEvent)
                m_threadStopEvent = m_consumer->listen("consumer-thread-stopped", this, (mlt_listener) onThreadStopped);
            if (!serviceName.startsWith("decklink") && !isMulti)
                m_consumer->set("mlt_image_format", "glsl");
        } else {
            emit started();
        }
    }
    else {
        // Cleanup on error
        error = 2;
        Controller::closeConsumer();
        Controller::close();
    }
    return error;
}

QPoint GLWidget::offset() const
{
    return QPoint(m_offset.x() - (MLT.profile().width()  * m_zoom -  width()) / 2,
                  m_offset.y() - (MLT.profile().height() * m_zoom - height()) / 2);
}

void GLWidget::onFrameDisplayed(const SharedFrame &frame)
{
    m_mutex.lock();
    m_sharedFrame = frame;
    m_mutex.unlock();
    emit textureUpdated();
}

void GLWidget::setZoom(float zoom)
{
    m_zoom = zoom;
    emit zoomChanged();
    update();
}

void GLWidget::setOffsetX(int x)
{
    m_offset.setX(x);
    emit offsetChanged();
    update();
}

void GLWidget::setOffsetY(int y)
{
    m_offset.setY(y);
    emit offsetChanged();
    update();
}

void GLWidget::setCurrentFilter(QmlFilter* filter, QmlMetadata* meta)
{
    rootContext()->setContextProperty("filter", filter);
    if (meta && QFile::exists(meta->vuiFilePath().toLocalFile())) {
        setSource(meta->vuiFilePath());
    } else {
        setBlankScene();
    }
}

void GLWidget::updateTexture(GLuint yName, GLuint uName, GLuint vName)
{
    m_texture[0] = yName;
    m_texture[1] = uName;
    m_texture[2] = vName;
    emit textureUpdated();
}

// MLT consumer-frame-show event handler
void GLWidget::on_frame_show(mlt_consumer, void* self, mlt_frame frame_ptr)
{
    Mlt::Frame frame(frame_ptr);
    if (frame.get_int("rendered")) {
        GLWidget* widget = static_cast<GLWidget*>(self);
        int timeout = (widget->consumer()->get_int("real_time") > 0)? 0: 1000;
        if (widget->m_frameRenderer && widget->m_frameRenderer->semaphore()->tryAcquire(1, timeout)) {
            QMetaObject::invokeMethod(widget->m_frameRenderer, "showFrame", Qt::QueuedConnection, Q_ARG(Mlt::Frame, frame));
        }
    }
}

void GLWidget::onOpenGLContextCreated(QOpenGLContext* context)
{
    m_offscreenSurface.setFormat(context->format());
    m_offscreenSurface.create();
}

RenderThread::RenderThread(thread_function_t function, void *data, QOpenGLContext *context, QSurface* surface)
    : QThread(0)
    , m_function(function)
    , m_data(data)
    , m_context(0)
    , m_surface(surface)
{
    if (context) {
        m_context = new QOpenGLContext;
        m_context->setFormat(context->format());
        m_context->setShareContext(context);
        m_context->create();
        m_context->moveToThread(this);
    }
}

void RenderThread::run()
{
    if (m_context) {
        m_context->makeCurrent(m_surface);
    }
    m_function(m_data);
    if (m_context) {
        m_context->doneCurrent();
        delete m_context;
    }
}

FrameRenderer::FrameRenderer(QOpenGLContext* shareContext, QSurface* surface)
     : QThread(0)
     , m_semaphore(3)
     , m_frame()
     , m_context(0)
     , m_surface(surface)
     , m_gl32(0)
{
    Q_ASSERT(shareContext);
    m_renderTexture[0] = m_renderTexture[1] = m_renderTexture[2] = 0;
    m_displayTexture[0] = m_displayTexture[1] = m_displayTexture[2] = 0;
    if (shareContext->supportsThreadedOpenGL()) {
        m_context = new QOpenGLContext;
        m_context->setFormat(shareContext->format());
        m_context->setShareContext(shareContext);
        m_context->create();
        m_context->moveToThread(this);
    }
    setObjectName("FrameRenderer");
    moveToThread(this);
    start();
}

FrameRenderer::~FrameRenderer()
{
    qDebug();
    delete m_context;
    delete m_gl32;
}

void FrameRenderer::showFrame(Mlt::Frame frame)
{
    int width = 0;
    int height = 0;

    if (!Settings.playerGPU()) {
        // Convert the image format before creating the SharedFrame.
        mlt_image_format format = mlt_image_yuv420p;
        int width = 0;
        int height = 0;
        frame.get_image(format, width, height);
        m_frame = SharedFrame(frame);
    }

    if (m_context && m_context->isValid()) {
        if (Settings.playerGPU()) {
            frame.set("movit.convert.use_texture", 1);
            mlt_image_format format = mlt_image_glsl_texture;
            const GLuint* textureId = (GLuint*) frame.get_image(format, width, height);

            m_context->makeCurrent(m_surface);
#ifdef USE_GL_SYNC
            GLsync sync = (GLsync) frame.get_data("movit.convert.fence");
            if (sync) {
#ifdef Q_OS_WIN
                // On Windows, use QOpenGLFunctions_3_2_Core instead of getProcAddress.
                if (!m_gl32) {
                    m_gl32 = m_context->versionFunctions<QOpenGLFunctions_3_2_Core>();
                    if (m_gl32)
                        m_gl32->initializeOpenGLFunctions();
                }
                if (m_gl32) {
                    m_gl32->glClientWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
                    check_error(m_gl32);
                }
#else
                if (ClientWaitSync) {
                    ClientWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
                    check_error(m_context->functions());
                }
#endif // Q_OS_WIN
            }
#else
            m_context->functions()->glFinish();
#endif // USE_GL_FENCE
            emit textureReady(*textureId);
            m_context->doneCurrent();

            // Save this frame for future use and to keep a reference to the GL Texture.
            m_frame = SharedFrame(frame);
        }
        else {
            // Using a threaded OpenGL to upload textures.
            m_context->makeCurrent(m_surface);
            QOpenGLFunctions* f = m_context->functions();

            uploadTextures(m_context, m_frame, m_renderTexture);
            f->glBindTexture(GL_TEXTURE_2D, 0);
            check_error(f);
            f->glFinish();

            for (int i = 0; i < 3; ++i)
                qSwap(m_renderTexture[i], m_displayTexture[i]);
            emit textureReady(m_displayTexture[0], m_displayTexture[1], m_displayTexture[2]);
            m_context->doneCurrent();
        }
    }

    // The frame is now done being modified and can be shared with the rest
    // of the application.
    emit frameDisplayed(m_frame);

    m_semaphore.release();
}

SharedFrame FrameRenderer::getDisplayFrame()
{
    return m_frame;
}

void FrameRenderer::cleanup()
{
    qDebug();
    if (m_renderTexture[0] && m_renderTexture[1] && m_renderTexture[2]) {
        m_context->makeCurrent(m_surface);
        m_context->functions()->glDeleteTextures(3, m_renderTexture);
        if (m_displayTexture[0] && m_displayTexture[1] && m_displayTexture[2])
            m_context->functions()->glDeleteTextures(3, m_displayTexture);
        m_context->doneCurrent();
        m_renderTexture[0] = m_renderTexture[1] = m_renderTexture[2] = 0;
        m_displayTexture[0] = m_displayTexture[1] = m_displayTexture[2] = 0;
    }
}
