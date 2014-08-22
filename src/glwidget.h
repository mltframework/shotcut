/*
 * Copyright (c) 2011-2014 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QSemaphore>
#include <QQuickView>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QRect>
#include "mltcontroller.h"

class QOpenGLFunctions_3_2_Core;
class QOffscreenSurface;
class QOpenGLTexture;

namespace Mlt {

class Filter;
class RenderThread;
class FrameRenderer;

typedef void* ( *thread_function_t )( void* );

class GLWidget : public QQuickView, public Controller, protected QOpenGLFunctions
{
    Q_OBJECT
    Q_PROPERTY(QRect rect READ rect NOTIFY rectChanged)
    Q_PROPERTY(float zoom READ zoom NOTIFY zoomChanged)
    Q_PROPERTY(QPoint offset READ offset NOTIFY offsetChanged)

public:
    GLWidget(QObject *parent = 0);
    ~GLWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void createThread(RenderThread** thread, thread_function_t function, void* data);
    void startGlsl();
    void stopGlsl();
    int setProducer(Mlt::Producer*, bool isMulti = false);
    int reconfigure(bool isMulti);
    void setLastFrame(mlt_frame frame);

    void play(double speed = 1.0) {
        Controller::play(speed);
        if (speed == 0) emit paused();
        else emit playing();
    }
    void seek(int position) {
        Controller::seek(position);
        emit paused();
    }
    void pause() {
        Controller::pause();
        emit paused();
    }
    int displayWidth() const { return m_rect.width(); }
    int displayHeight() const { return m_rect.height(); }

    QObject* videoWidget() { return this; }
    QQuickView* videoQuickView() { return this; }
    Filter* glslManager() const { return m_glslManager; }
    FrameRenderer* frameRenderer() const { return m_frameRenderer; }
    QRect rect() const { return m_rect; }
    float zoom() const { return m_zoom * MLT.profile().width() / m_rect.width(); }
    QPoint offset() const;

public slots:
    void setZoom(float zoom);
    void setOffsetX(int x);
    void setOffsetY(int y);

signals:
    /** This method will be called each time a new frame is available.
     * @param frame a Mlt::QFrame from which to get a QImage
     */
    void frameReceived(Mlt::QFrame frame);
    void textureUpdated();
    void dragStarted();
    void seekTo(int x);
    void gpuNotSupported();
    void started();
    void paused();
    void playing();
    void rectChanged();
    void zoomChanged();
    void offsetChanged();

private:
    QRect m_rect;
    GLuint m_texture[3];
    QOpenGLShaderProgram* m_shader;
    QPoint m_dragStart;
    Filter* m_glslManager;
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_isInitialized;
    Event* m_threadStartEvent;
    Event* m_threadStopEvent;
    Mlt::Frame* m_lastFrame;
    Event* m_threadCreateEvent;
    Event* m_threadJoinEvent;
    FrameRenderer* m_frameRenderer;
    int m_projectionLocation;
    int m_modelViewLocation;
    int m_vertexLocation;
    int m_texCoordLocation;
    int m_colorspaceLocation;
    int m_textureLocation[3];
    float m_zoom;
    QPoint m_offset;

private slots:
    void initializeGL();
    void resizeGL(int width, int height);
    void updateTexture(GLuint yName, GLuint uName, GLuint vName);
    void paintGL();

protected:
    void resizeEvent(QResizeEvent* event);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void createShader();

    static void on_frame_show(mlt_consumer, void* self, mlt_frame frame);
};

class RenderThread : public QThread
{
    Q_OBJECT
public:
    RenderThread(thread_function_t function, void* data, QOpenGLContext *context);

protected:
    void run();

private:
    thread_function_t m_function;
    void* m_data;
    QOpenGLContext* m_context;
    QOffscreenSurface* m_surface;
};

class FrameRenderer : public QThread
{
    Q_OBJECT
public:
    FrameRenderer(QOpenGLContext* shareContext);
    ~FrameRenderer();
    QSemaphore* semaphore() { return &m_semaphore; }
    QOpenGLContext* context() const { return m_context; }

public slots:
    void showFrame(Mlt::QFrame frame);
    void cleanup();

signals:
    void textureReady(GLuint yName, GLuint uName = 0, GLuint vName = 0);

private:
    QSemaphore m_semaphore;
    QOpenGLContext* m_context;
    QOffscreenSurface* m_surface;
public:
    GLuint m_renderTexture[3];
    GLuint m_displayTexture[3];
    QOpenGLFunctions_3_2_Core* m_gl32;
    Frame* m_frame;
};

} // namespace

#endif
