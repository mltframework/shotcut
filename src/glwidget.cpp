/*
 * Copyright (c) 2011-2014 Meltytech, LLC
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
#include <Mlt.h>
#include "glwidget.h"
#include "settings.h"

#define check_error() { int err = glGetError(); if (err != GL_NO_ERROR) { qCritical() << "GL error"  << err << "at %s" << __FILE__ << ":" << __LINE__; } }

#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFFull
#endif

#ifndef Q_OS_WIN
typedef GLenum (*ClientWaitSync_fp) (GLsync sync, GLbitfield flags, GLuint64 timeout);
static ClientWaitSync_fp ClientWaitSync = 0;
#endif

using namespace Mlt;

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(parent)
    , Controller()
    , showFrameSemaphore(3)
    , m_image_width(0)
    , m_image_height(0)
    , m_display_ratio(4.0/3.0)
    , m_shader(0)
    , m_glslManager(0)
    , m_fbo(0)
    , m_isInitialized(false)
    , m_threadStartEvent(0)
    , m_threadStopEvent(0)
    , m_image_format(mlt_image_yuv422)
    , m_lastFrame(0)
    , m_threadCreateEvent(0)
    , m_threadJoinEvent(0)
    , m_gl32(0)
{
    m_texture[0] = m_texture[1] = m_texture[2] = 0;
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);
    if (Settings.playerGPU())
        m_glslManager = new Filter(profile(), "glsl.manager");
    if ((m_glslManager && !m_glslManager->is_valid())) {
        delete m_glslManager;
        m_glslManager = 0;
    }
#ifdef Q_OS_WIN
    if (m_glslManager) {
        // On Windows, my NVIDIA card needs me to set the OpenGL version to
        // handle the fancy GL functions that Movit uses.
        QGLFormat format;
        format.setVersion(3, 2);
        format.setProfile(QGLFormat::CompatibilityProfile);
        setFormat(format);
    }
#endif
}

GLWidget::~GLWidget()
{
    stop();
    makeCurrent();
    if (m_texture[0] && !m_glslManager)
        glDeleteTextures(3, m_texture);
    delete m_fbo;
    delete m_glslManager;
    delete m_threadStartEvent;
    delete m_threadStopEvent;
    delete m_threadCreateEvent;
    delete m_threadJoinEvent;
    delete m_lastFrame;
    delete m_gl32;
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(40, 30);
}

QSize GLWidget::sizeHint() const
{
    return QSize(400, 300);
}

void GLWidget::initializeGL()
{
    QPalette palette;

#ifndef Q_OS_WIN
    // getProcAddress is not working for me on Windows.
    if (Settings.playerGPU()) {
        QOpenGLContext* cx = context()->contextHandle();
        if (m_glslManager && cx->hasExtension("GL_ARB_sync")) {
            ClientWaitSync = (ClientWaitSync_fp) cx->getProcAddress("glClientWaitSync");
        }
        if (!ClientWaitSync) {
            emit gpuNotSupported();
            delete m_glslManager;
            m_glslManager = 0;
        }
    }
#endif

    initializeOpenGLFunctions();
    qglClearColor(palette.color(QPalette::Window));
    glShadeModel(GL_FLAT);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_DITHER);
    glDisable(GL_BLEND);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    m_condition.wakeAll();
    m_isInitialized = true;
}

void GLWidget::resizeGL(int width, int height)
{
    double this_aspect = (double) width / height;

    // Special case optimisation to negate odd effect of sample aspect ratio
    // not corresponding exactly with image resolution.
    if ((int) (this_aspect * 1000) == (int) (m_display_ratio * 1000))
    {
        w = width;
        h = height;
    }
    // Use OpenGL to normalise sample aspect ratio
    else if (height * m_display_ratio > width)
    {
        w = width;
        h = width / m_display_ratio;
    }
    else
    {
        w = height * m_display_ratio;
        h = height;
    }
    x = (width - w) / 2;
    y = (height - h) / 2;

    if (isValid() && m_isInitialized) {
        glViewport(0, 0, width, height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}

void GLWidget::resizeEvent(QResizeEvent* event)
{
    // Following are needed to optimize video for retina Macs!
    int width = event->size().width() * devicePixelRatio();
    int height = event->size().height() * devicePixelRatio();
    resizeGL(width, height);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (m_texture[0]) {
        if (m_glslManager && m_fbo && m_image_format == mlt_image_glsl_texture) {
            glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
            check_error();
        }
        glBegin(GL_QUADS);
            glTexCoord2i(0, 0); glVertex2i(x, y);
            glTexCoord2i(1, 0); glVertex2i(x + w, y);
            glTexCoord2i(1, 1); glVertex2i(x + w, y + h);
            glTexCoord2i(0, 1); glVertex2i(x, y + h);
        glEnd();
    }
}

void GLWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStart = event->pos();
    if (MLT.isClip())
        emit dragStarted();
}

void GLWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event->modifiers() == Qt::ShiftModifier && m_producer) {
        emit seekTo(m_producer->get_length() * event->x() / width());
        return;
    }
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - m_dragStart).manhattanLength() < QApplication::startDragDistance())
        return;
    if (!MLT.isClip())
        return;
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(Mlt::XmlMimeType, MLT.saveXML("string").toUtf8());
    drag->setMimeData(mimeData);
    mimeData->setText(QString::number(MLT.producer()->get_playtime()));
    if (m_lastFrame && !Settings.playerGPU())
        drag->setPixmap(QPixmap::fromImage(MLT.image(m_lastFrame, 45 * MLT.profile().dar(), 45)).scaledToHeight(45));
    drag->setHotSpot(QPoint(0, 0));
    drag->exec(Qt::LinkAction);
}

void GLWidget::createShader()
{
    if (!m_shader) {
        makeCurrent();
        m_shader = new QOpenGLShaderProgram(this);
        m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment,
        "uniform sampler2D Ytex, Utex, Vtex;"
        "uniform int colorspace;"
        "void main(void) {"
        "  float x = gl_TexCoord[0].x;"
        "  float y = gl_TexCoord[0].y;"
        "  vec4 input;"
        "  input.r = texture2D(Ytex, vec2(x, y)).r - 0.0625;" // Y
        "  input.g = texture2D(Utex, vec2(x, y)).r - 0.5;"    // U
        "  input.b = texture2D(Vtex, vec2(x, y)).r - 0.5;"    // V
        "  input.a  = 1.0;"
        "  mat4 coefficients;"
        "  if (colorspace == 601) {"
        "    coefficients = mat4("
        "      1.1643,  1.1643,  1.1643, 0.0," // column 1
        "      0.0,    -0.39173, 2.017,  0.0," // column 2
        "      1.5958, -0.8129,  0.0,    0.0," // column 3
        "      0.0,     0.0,     0.0,    1.0);"// column 4
        "  } else {" // ITU-R 709
        "    coefficients = mat4("
        "      1.1643, 1.1643, 1.1643, 0.0," // column 1
        "      0.0,   -0.213,  2.112,  0.0," // column 2
        "      1.793, -0.533,  0.0,    0.0," // column 3
        "      0.0,    0.0,    0.0,    1.0);"// column 4
        "  }"
        "  gl_FragColor = coefficients * input;"
        "}");
        m_shader->bind();
        doneCurrent();
    }
}

void GLWidget::destroyShader()
{
    if (m_shader) {
        makeCurrent();
        m_shader->release();
        delete m_shader;
        m_shader = 0;
        doneCurrent();
    }
}

void GLWidget::createThread(RenderThread **thread, thread_function_t function, void *data)
{
#ifdef Q_OS_WIN
    // On Windows, MLT event consumer-thread-create is fired from the Qt main thread.
    while (!m_isInitialized)
        qApp->processEvents();
#else
    if (!m_isInitialized) {
        m_mutex.lock();
        m_condition.wait(&m_mutex);
        m_mutex.unlock();
    }
#endif
    (*thread) = new RenderThread(function, data, m_glslManager? context()->contextHandle() : 0);
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
    m_glslManager->fire_event("close glsl");
    m_texture[0] = 0;
}

static void onThreadStopped(mlt_properties owner, GLWidget* self)
{
    Q_UNUSED(owner)
    self->stopGlsl();
}

void GLWidget::showFrame(Mlt::QFrame frame)
{
    if (m_isInitialized && frame.frame()->get_int("rendered")) {
        m_image_width = 0;
        m_image_height = 0;
        makeCurrent();
        if (m_glslManager && m_image_format == mlt_image_glsl_texture) {
            frame.frame()->set("movit.convert.use_texture", 1);
            const GLuint* textureId = (GLuint*) frame.frame()->get_image(m_image_format, m_image_width, m_image_height);
            m_image_format = mlt_image_glsl_texture;
            m_texture[0] = *textureId;

            if (!m_fbo || m_fbo->width() != m_image_width || m_fbo->height() != m_image_height) {
                delete m_fbo;
                m_fbo = new QOpenGLFramebufferObject(m_image_width, m_image_height, GL_TEXTURE_2D);
            }
            glPushAttrib(GL_VIEWPORT_BIT);
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();

            glBindFramebuffer( GL_FRAMEBUFFER, m_fbo->handle());
            check_error();

            glViewport( 0, 0, m_image_width, m_image_height );
            glMatrixMode( GL_PROJECTION );
            glLoadIdentity();
            glOrtho(0.0, m_image_width, 0.0, m_image_height, -1.0, 1.0);
            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();

            GLsync sync = (GLsync) frame.frame()->get_data("movit.convert.fence");
            if (sync) {
#ifdef Q_OS_WIN
                // On Windows, use QOpenGLFunctions_3_2_Core instead of getProcAddress.
                if (!m_gl32) {
                    m_gl32 = context()->contextHandle()->versionFunctions<QOpenGLFunctions_3_2_Core>();
                    if (m_gl32)
                        m_gl32->initializeOpenGLFunctions();
                }
                if (m_gl32) {
                    m_gl32->glClientWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
                    check_error();
                }
#else
                if (ClientWaitSync) {
                    ClientWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
                    check_error();
                }
#endif
            }

            glActiveTexture( GL_TEXTURE0 );
            check_error();
            glBindTexture( GL_TEXTURE_2D, m_texture[0]);
            check_error();

            glBegin( GL_QUADS );
                glTexCoord2i( 0, 0 ); glVertex2i( 0, 0 );
                glTexCoord2i( 0, 1 ); glVertex2i( 0, m_image_height );
                glTexCoord2i( 1, 1 ); glVertex2i( m_image_width, m_image_height );
                glTexCoord2i( 1, 0 ); glVertex2i( m_image_width, 0 );
            glEnd();
            check_error();

            glBindFramebuffer( GL_FRAMEBUFFER, 0 );
            check_error();
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopAttrib();
        }
        else if (m_shader) {
            m_image_format = mlt_image_yuv420p;
            const uint8_t* image = frame.frame()->get_image(m_image_format, m_image_width, m_image_height);

            // Copy each plane of YUV to a texture bound to shader program˙.
            if (m_texture[0])
                glDeleteTextures(3, m_texture);
            glPixelStorei  (GL_UNPACK_ROW_LENGTH, m_image_width);
            glGenTextures  (3, m_texture);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture  (GL_TEXTURE_2D, m_texture[0]);
            m_shader->setUniformValue(m_shader->uniformLocation("Ytex"), 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D   (GL_TEXTURE_2D, 0, GL_LUMINANCE, m_image_width, m_image_height, 0,
                            GL_LUMINANCE, GL_UNSIGNED_BYTE, image);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture  (GL_TEXTURE_2D, m_texture[1]);
            m_shader->setUniformValue(m_shader->uniformLocation("Utex"), 1);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D   (GL_TEXTURE_2D, 0, GL_LUMINANCE, m_image_width/2, m_image_height/4, 0,
                            GL_LUMINANCE, GL_UNSIGNED_BYTE, image + m_image_width * m_image_height);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture  (GL_TEXTURE_2D, m_texture[2]);
            m_shader->setUniformValue(m_shader->uniformLocation("Vtex"), 2);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D   (GL_TEXTURE_2D, 0, GL_LUMINANCE, m_image_width/2, m_image_height/4, 0,
                            GL_LUMINANCE, GL_UNSIGNED_BYTE, image + m_image_width * m_image_height + m_image_width/2 * m_image_height/2);

            m_shader->setUniformValue(m_shader->uniformLocation("colorspace"), MLT.profile().colorspace());

            delete m_lastFrame;
            m_lastFrame = new Mlt::Frame(*frame.frame());
        }
        glDraw();
    }
    showFrameSemaphore.release();
}

int GLWidget::setProducer(Mlt::Producer* producer, bool isMulti)
{
    int error = Controller::setProducer(producer, isMulti);

    delete m_lastFrame;
    m_lastFrame = 0;

    if (!error) {
        bool reconnect = !m_consumer || !m_consumer->is_valid();
        error = reconfigure(isMulti);
        if (!error) {
            if (reconnect)
                connect(this, SIGNAL(frameReceived(Mlt::QFrame)),
                        this, SLOT(showFrame(Mlt::QFrame)), Qt::UniqueConnection);
            resizeGL(width() * devicePixelRatio(), height() * devicePixelRatio());
        }
    }
    return error;
}

int GLWidget::reconfigure(bool isMulti)
{
    int error = 0;

    delete m_lastFrame;
    m_lastFrame = 0;

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

        Mlt::Filter* filter = new Mlt::Filter(profile(), "audiolevel");
        if (filter->is_valid())
            m_consumer->attach(*filter);
        delete filter;
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
        if (m_glslManager) {
            m_consumer->set("real_time", property("realtime").toBool()? 1 : -1);
        } else {
            int threadCount = QThread::idealThreadCount();
            threadCount = threadCount > 2? (threadCount > 3? 3 : 2) : 1;
            m_consumer->set("real_time", property("realtime").toBool()? 1 : -threadCount);
        }
        m_consumer->set("mlt_image_format", "yuv422");
        m_display_ratio = profile().dar();

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
            m_consumer->set("scrub_audio", 1);
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
            m_image_format = mlt_image_glsl_texture;
            destroyShader();
        } else {
            m_image_format = mlt_image_yuv420p;
            createShader();
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

// MLT consumer-frame-show event handler
void GLWidget::on_frame_show(mlt_consumer, void* self, mlt_frame frame_ptr)
{
    GLWidget* widget = static_cast<GLWidget*>(self);
    if (widget->showFrameSemaphore.tryAcquire()) {
        Frame frame(frame_ptr);
        emit widget->frameReceived(Mlt::QFrame(frame));
    }
}

RenderThread::RenderThread(thread_function_t function, void *data, QOpenGLContext *context)
    : QThread(0)
    , m_function(function)
    , m_data(data)
    , m_context(0)
    , m_surface(context? context->surface() : 0)
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
