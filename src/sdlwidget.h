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

#ifndef SDLWIDGET_H
#define SDLWIDGET_H

#include <QWidget>
#include "mltcontroller.h"

namespace Mlt {

class SDLWidget : public QWidget, public Controller
{
    Q_OBJECT
public:
    explicit SDLWidget(QWidget *parent = 0);
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
    int displayWidth() const { return width(); }
    int displayHeight() const { return height(); }

    QWidget* videoWidget() { return this; }

signals:
    /** This method will be called each time a new frame is available.
     * @param frame a Mlt::QFrame from which to get a QImage
     * @param position the frame number of this frame representing time
     */
    void frameReceived(Mlt::QFrame);
    void dragStarted();
    void seekTo(int x);
    void started();
    void paused();

public slots:

private:
    QPoint m_dragStart;

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    static void on_frame_show(mlt_consumer, void* self, mlt_frame frame);

};

} //namespace

#endif // SDLWIDGET_H
