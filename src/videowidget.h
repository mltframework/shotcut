/*
 * Copyright (c) 2011-2025 Meltytech, LLC
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

#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include "mltcontroller.h"
#include "settings.h"
#include "sharedframe.h"

#include <QMutex>
#include <QQuickWidget>
#include <QRectF>
#include <QSemaphore>
#include <QThread>
#include <QTimer>

class QmlFilter;
class QmlMetadata;
class QOpenGLContext;
class QOffscreenSurface;

namespace Mlt {

class Filter;
class RenderThread;
class FrameRenderer;

typedef void *(*thread_function_t)(void *);

class VideoWidget : public QQuickWidget, public Controller
{
    Q_OBJECT
    Q_PROPERTY(QRectF rect READ rect NOTIFY rectChanged)
    Q_PROPERTY(int grid READ grid NOTIFY gridChanged)
    Q_PROPERTY(bool snapToGrid READ snapToGrid NOTIFY snapToGridChanged)
    Q_PROPERTY(float zoom READ zoom NOTIFY zoomChanged)
    Q_PROPERTY(QPoint offset READ offset NOTIFY offsetChanged)

public:
    VideoWidget(QObject *parent = 0);
    virtual ~VideoWidget();

    int setProducer(Mlt::Producer *, bool isMulti = false) override;
    void createThread(RenderThread **thread, thread_function_t function, void *data);
    void startGlsl();
    void stopGlsl();
    int reconfigure(bool isMulti) override;

    void play(double speed = 1.0) override
    {
        Controller::play(speed);
        if (speed == 0)
            emit paused();
        else
            emit playing();
    }
    void seek(int position) override
    {
        Controller::seek(position);
        if (Settings.playerPauseAfterSeek())
            emit paused();
    }
    void refreshConsumer(bool scrubAudio = false) override;
    void pause(int position = -1) override
    {
        Controller::pause();
        emit paused();
    }
    int displayWidth() const override { return m_rect.width(); }
    int displayHeight() const override { return m_rect.height(); }

    QObject *videoWidget() override { return this; }
    QRectF rect() const { return m_rect; }
    int grid() const { return m_grid; }
    float zoom() const { return m_zoom * MLT.profile().width() / m_rect.width(); }
    QPoint offset() const;
    QImage image() const;
    bool imageIsProxy() const;
    void requestImage() const;
    bool snapToGrid() const { return m_snapToGrid; }
    int maxTextureSize() const { return m_maxTextureSize; }
    void toggleVuiDisplay();

public slots:
    void setGrid(int grid);
    void setZoom(float zoom);
    void setOffsetX(int x);
    void setOffsetY(int y);
    void setBlankScene();
    void setCurrentFilter(QmlFilter *filter, QmlMetadata *meta);
    void setSnapToGrid(bool snap);
    virtual void initialize();
    virtual void beforeRendering(){};
    virtual void renderVideo();
    virtual void onFrameDisplayed(const SharedFrame &frame);

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
    void offsetChanged(const QPoint &offset = QPoint());
    void imageReady();
    void snapToGridChanged();
    void toggleZoom(bool);
    void stepZoom(float, float);

private:
    QRectF m_rect;
    int m_grid;
    QPoint m_dragStart;
    QSemaphore m_initSem;
    bool m_isInitialized;
    std::unique_ptr<Filter> m_glslManager;
    std::unique_ptr<Event> m_threadStartEvent;
    std::unique_ptr<Event> m_threadStopEvent;
    std::unique_ptr<Event> m_threadCreateEvent;
    std::unique_ptr<Event> m_threadJoinEvent;
    FrameRenderer *m_frameRenderer;
    float m_zoom;
    QPoint m_offset;
    QUrl m_savedQmlSource;
    bool m_hideVui;
    bool m_snapToGrid;
    QTimer m_refreshTimer;
    bool m_scrubAudio;
    QPoint m_mousePosition;
    std::unique_ptr<RenderThread> m_renderThread;

    static void on_frame_show(mlt_consumer, VideoWidget *widget, mlt_event_data);

private slots:
    void resizeVideo(int width, int height);
    void onRefreshTimeout();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;
    void createShader();

    int m_maxTextureSize;
    SharedFrame m_sharedFrame;
    QMutex m_mutex;
};

class RenderThread : public QThread
{
    Q_OBJECT
public:
    RenderThread(thread_function_t function, void *data);
    ~RenderThread();

protected:
    void run();

private:
    thread_function_t m_function;
    void *m_data;
    std::unique_ptr<QOpenGLContext> m_context;
    std::unique_ptr<QOffscreenSurface> m_surface;
};

class FrameRenderer : public QThread
{
    Q_OBJECT
public:
    FrameRenderer();
    ~FrameRenderer();
    QSemaphore *semaphore() { return &m_semaphore; }
    SharedFrame getDisplayFrame();
    Q_INVOKABLE void showFrame(Mlt::Frame frame);
    void requestImage();
    QImage image() const { return m_image; }

signals:
    void frameDisplayed(const SharedFrame &frame);
    void imageReady();

private:
    QSemaphore m_semaphore;
    SharedFrame m_displayFrame;
    bool m_imageRequested;
    QImage m_image;
};

} // namespace Mlt

#endif
