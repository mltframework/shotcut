/*
 * Copyright (c) 2011 Meltytech, LLC
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
#include <QGLFunctions>
#include <QGLWidget>
#include <QGLShaderProgram>
#include <QGLFramebufferObject>
#include <QMutex>
#include <QWaitCondition>
#include "mltcontroller.h"

namespace Mlt {

class Filter;

class GLWidget : public QGLWidget, public Controller, protected QGLFunctions
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent = 0);
    ~GLWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void startGlsl();
    void stopGlsl();
    int open(Mlt::Producer*, bool isMulti = false);
    int reconfigure(bool isMulti);

    void play(double speed = 1.0) {
        Controller::play(speed);
        if (speed == 0) emit paused();
    }
    void seek(int position) {
        Controller::seek(position);
        emit paused();
    }
    void pause() {
        Controller::pause();
        emit paused();
    }

    QWidget* videoWidget() { return this; }
    QSemaphore showFrameSemaphore;

public slots:
    void showFrame(Mlt::QFrame);

signals:
    /** This method will be called each time a new frame is available.
     * @param frame a Mlt::QFrame from which to get a QImage
     */
    void frameReceived(Mlt::QFrame frame);
    void dragStarted();
    void seekTo(int x);
    void gpuNotSupported();
    void started();
    void paused();

private:
    int x, y, w, h;
    int m_image_width, m_image_height;
    GLuint m_texture[3];
    double m_display_ratio;
    QGLShaderProgram* m_shader;
    QPoint m_dragStart;
    Filter* m_glslManager;
    QGLWidget* m_renderContext;
    QGLFramebufferObject* m_fbo;
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_isInitialized;
    Event* m_threadStartEvent;
    Event* m_threadStopEvent;
    mlt_image_format m_image_format;

protected:
    void initializeGL();
    void resizeGL(int width, int height);
    void resizeEvent(QResizeEvent* event);
    void paintGL();
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void createShader();
    void destroyShader();

    static void on_frame_show(mlt_consumer, void* self, mlt_frame frame);
};

} // namespace

#endif
