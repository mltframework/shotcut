/*
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

#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QSemaphore>
#include <QQuickWidget>
#include <QMutex>
#include <QThread>
#include <QRectF>
#include <QTimer>
#include "mltcontroller.h"
#include "sharedframe.h"

class QmlFilter;
class QmlMetadata;

namespace Mlt {

class FrameRenderer;

typedef void *( *thread_function_t )( void * );

class VideoWidget : public QQuickWidget, public Controller/*, protected QOpenGLFunctions*/
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
    void setGrid(int grid);
    void setZoom(float zoom);
    void setOffsetX(int x);
    void setOffsetY(int y);
    void setBlankScene();
    void setCurrentFilter(QmlFilter *filter, QmlMetadata *meta);
    void setSnapToGrid(bool snap);
    virtual void initialize();
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

private:
    QRectF m_rect;
    int m_grid;
    unsigned int m_texture[3];
    QPoint m_dragStart;
    QSemaphore m_initSem;
    bool m_isInitialized;
    FrameRenderer *m_frameRenderer;
    float m_zoom;
    QPoint m_offset;
    QUrl m_savedQmlSource;
    bool m_snapToGrid;
    QTimer m_refreshTimer;
    bool m_scrubAudio;
    QPoint m_mousePosition;


    static void on_frame_show(mlt_consumer, VideoWidget *widget, mlt_event_data);

private slots:
    void resizeVideo(int width, int height);
    void updateTexture(unsigned int yName, unsigned int uName, unsigned int vName);
    void onRefreshTimeout();

protected:
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *event);
    bool event(QEvent *event);
    void createShader();

    int m_maxTextureSize;
    SharedFrame m_sharedFrame;
    QMutex m_mutex;
};

class FrameRenderer : public QThread
{
    Q_OBJECT
public:
    FrameRenderer();
    ~FrameRenderer();
    QSemaphore *semaphore()
    {
        return &m_semaphore;
    }
    SharedFrame getDisplayFrame();
    Q_INVOKABLE void showFrame(Mlt::Frame frame);
    void requestImage();
    QImage image() const
    {
        return m_image;
    }

signals:
    void textureReady(unsigned int yName, unsigned int uName = 0, unsigned int vName = 0);
    void frameDisplayed(const SharedFrame &frame);
    void imageReady();

private:
    QSemaphore m_semaphore;
    SharedFrame m_displayFrame;
    bool m_imageRequested;
    QImage m_image;
};

} // namespace

#endif
