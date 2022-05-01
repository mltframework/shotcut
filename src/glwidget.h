/*
 * Copyright (c) 2011-2020 Meltytech, LLC
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
#include <QQuickWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QMutex>
#include <QThread>
#include <QRectF>
#include <QTimer>
#include "mltcontroller.h"
#include "sharedframe.h"

class QOpenGLFunctions_3_2_Core;
class QOpenGLTexture;
class QmlFilter;
class QmlMetadata;

namespace Mlt {

class Filter;
class RenderThread;
class FrameRenderer;

typedef void *( *thread_function_t )( void * );

class GLWidget : public QQuickWidget, public Controller, protected QOpenGLFunctions
{
    Q_OBJECT
    Q_PROPERTY(QRectF rect READ rect NOTIFY rectChanged)
    Q_PROPERTY(int grid READ grid NOTIFY gridChanged)
    Q_PROPERTY(bool snapToGrid READ snapToGrid NOTIFY snapToGridChanged)
    Q_PROPERTY(float zoom READ zoom NOTIFY zoomChanged)
    Q_PROPERTY(QPoint offset READ offset NOTIFY offsetChanged)

public:
    GLWidget(QObject *parent = 0);
    ~GLWidget();

    void createThread(RenderThread **thread, thread_function_t function, void *data);
    void startGlsl();
    void stopGlsl();
    int setProducer(Mlt::Producer *, bool isMulti = false);
    int reconfigure(bool isMulti);

    void play(double speed = 1.0)
    {
        Controller::play(speed);
        if (speed == 0) emit paused();
        else emit playing();
    }
    void seek(int position)
    {
        Controller::seek(position);
        emit paused();
    }
    void refreshConsumer(bool scrubAudio = false);
    void pause()
    {
        Controller::pause();
        emit paused();
    }
    int displayWidth() const
    {
        return m_rect.width();
    }
    int displayHeight() const
    {
        return m_rect.height();
    }

    QObject *videoWidget()
    {
        return this;
    }
    Filter *glslManager() const
    {
        return m_glslManager;
    }
    QRectF rect() const
    {
        return m_rect;
    }
    int grid() const
    {
        return m_grid;
    }
    float zoom() const
    {
        return m_zoom * MLT.profile().width() / m_rect.width();
    }
    QPoint offset() const;
    QImage image() const;
    bool imageIsProxy() const;
    void requestImage() const;
    bool snapToGrid() const
    {
        return m_snapToGrid;
    }
    int maxTextureSize() const
    {
        return m_maxTextureSize;
    }

public slots:
    void onFrameDisplayed(const SharedFrame &frame);
    void setGrid(int grid);
    void setZoom(float zoom);
    void setOffsetX(int x);
    void setOffsetY(int y);
    void setBlankScene();
    void setCurrentFilter(QmlFilter *filter, QmlMetadata *meta);
    void setSnapToGrid(bool snap);

signals:
    void frameDisplayed(const SharedFrame &frame);
    void dragStarted();
    void seekTo(int x);
    void gpuNotSupported();
    void started();
    void paused();
    void playing();
    void rectChanged();
    void gridChanged();
    void zoomChanged();
    void offsetChanged();
    void imageReady();
    void snapToGridChanged();
    void toggleZoom(bool);

private:
    QRectF m_rect;
    int m_grid;
    GLuint m_texture[3];
    QOpenGLShaderProgram *m_shader;
    QPoint m_dragStart;
    Filter *m_glslManager;
    QSemaphore m_initSem;
    bool m_isInitialized;
    Event *m_threadStartEvent;
    Event *m_threadStopEvent;
    Event *m_threadCreateEvent;
    Event *m_threadJoinEvent;
    FrameRenderer *m_frameRenderer;
    int m_projectionLocation;
    int m_modelViewLocation;
    int m_vertexLocation;
    int m_texCoordLocation;
    int m_colorspaceLocation;
    int m_textureLocation[3];
    float m_zoom;
    QPoint m_offset;
    QOffscreenSurface m_offscreenSurface;
    QOpenGLContext *m_shareContext;
    SharedFrame m_sharedFrame;
    QMutex m_mutex;
    QUrl m_savedQmlSource;
    bool m_snapToGrid;
    QTimer m_refreshTimer;
    bool m_scrubAudio;
    GLint m_maxTextureSize;

    static void on_frame_show(mlt_consumer, GLWidget *widget, mlt_event_data);

private slots:
    void initializeGL();
    void resizeGL(int width, int height);
    void updateTexture(GLuint yName, GLuint uName, GLuint vName);
    void paintGL();
    void onRefreshTimeout();

protected:
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *event);
    bool event(QEvent *event);
    void createShader();
};

class RenderThread : public QThread
{
    Q_OBJECT
public:
    RenderThread(thread_function_t function, void *data, QOpenGLContext *context, QSurface *surface);

protected:
    void run();

private:
    thread_function_t m_function;
    void *m_data;
    QOpenGLContext *m_context;
    QSurface *m_surface;
};

class FrameRenderer : public QThread
{
    Q_OBJECT
public:
    FrameRenderer(QOpenGLContext *shareContext, QSurface *surface);
    ~FrameRenderer();
    QSemaphore *semaphore()
    {
        return &m_semaphore;
    }
    QOpenGLContext *context() const
    {
        return m_context;
    }
    SharedFrame getDisplayFrame();
    Q_INVOKABLE void showFrame(Mlt::Frame frame);
    void requestImage();
    QImage image() const
    {
        return m_image;
    }

public slots:
    void cleanup();

signals:
    void textureReady(GLuint yName, GLuint uName = 0, GLuint vName = 0);
    void frameDisplayed(const SharedFrame &frame);
    void imageReady();

private:
    QSemaphore m_semaphore;
    SharedFrame m_displayFrame;
    QOpenGLContext *m_context;
    QSurface *m_surface;
    qint64 m_previousMSecs;
    bool m_imageRequested;
    QImage m_image;

public:
    GLuint m_renderTexture[3];
    GLuint m_displayTexture[3];
    QOpenGLFunctions_3_2_Core *m_gl32;
};

} // namespace

#endif
