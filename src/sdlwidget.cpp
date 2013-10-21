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

#include "sdlwidget.h"
#include <Mlt.h>
#include <QtWidgets>

using namespace Mlt;

SDLWidget::SDLWidget(QWidget *parent)
    : QWidget(parent)
    , Controller()
{
    // Required for SDL embeddding
    parent->setAttribute(Qt::WA_NativeWindow);
    setMouseTracking(true);
}

int SDLWidget::setProducer(Mlt::Producer* producer, bool isMulti)
{
    int error = Controller::setProducer(producer, isMulti);

    if (!error)
        error = reconfigure(isMulti);
    return error;
}

int SDLWidget::reconfigure(bool isMulti)
{
    int error = 0;

    QString serviceName = property("mlt_service").toString();
    if (!m_consumer || !m_consumer->is_valid()) {
        if (serviceName.isEmpty())
#if defined(Q_OS_WIN)
            // sdl_preview does not work good on Windows
            serviceName = "sdl";
#else
            serviceName = "sdl_preview";
#endif
        if (isMulti)
            m_consumer = new Mlt::FilteredConsumer(profile(), "multi");
        else
            m_consumer = new Mlt::FilteredConsumer(profile(), serviceName.toLatin1().constData());

        Mlt::Filter* filter = new Mlt::Filter(profile(), "audiolevel");
        if (filter->is_valid())
            m_consumer->attach(*filter);
        delete filter;
    }
    if (m_consumer->is_valid()) {
        // Connect the producer to the consumer - tell it to "run" later
        m_consumer->connect(*m_producer);
        // Make an event handler for when a frame's image should be displayed
        m_consumer->listen("consumer-frame-show", this, (mlt_listener) on_frame_show);
        int threadCount = QThread::idealThreadCount();
        threadCount = threadCount > 2? (threadCount > 3? 3 : 2) : 1;
        m_consumer->set("real_time", property("realtime").toBool()? 1 : -threadCount);

        if (isMulti) {
            m_consumer->set("terminate_on_pause", 0);
            // Embed the SDL window in our GUI.
            m_consumer->set("0.window_id", (int) this->winId());
            // Set the background color
            m_consumer->set("0.window_background", palette().color(QPalette::Window).name().toLatin1().constData());
            if (!profile().progressive())
                m_consumer->set("progressive", property("progressive").toBool());
            m_consumer->set("0.rescale", property("rescale").toString().toLatin1().constData());
            m_consumer->set("0.deinterlace_method", property("deinterlace_method").toString().toLatin1().constData());
            m_consumer->set("0.buffer", 25);
            m_consumer->set("0.prefill", 1);
            m_consumer->set("0.play.buffer", 1);
#ifndef Q_OS_WIN
            m_consumer->set("0.scrub_audio", 1);
#endif
            if (property("keyer").isValid())
                m_consumer->set("0.keyer", property("keyer").toInt());
        }
        else {
            // Embed the SDL window in our GUI.
            m_consumer->set("window_id", (int) this->winId());
            // Set the background color
            m_consumer->set("window_background", palette().color(QPalette::Window).name().toLatin1().constData());
            if (!profile().progressive())
                m_consumer->set("progressive", property("progressive").toBool());
            m_consumer->set("rescale", property("rescale").toString().toLatin1().constData());
            m_consumer->set("deinterlace_method", property("deinterlace_method").toString().toLatin1().constData());
            m_consumer->set("buffer", 25);
            m_consumer->set("prefill", 1);
            m_consumer->set("play.buffer", 1);
#ifndef Q_OS_WIN
            m_consumer->set("scrub_audio", 1);
#endif
            if (property("keyer").isValid())
                m_consumer->set("keyer", property("keyer").toInt());
        }
        emit started();
    }
    else {
        // Cleanup on error
        error = 2;
        Controller::closeConsumer();
        Controller::close();
    }
    return error;
}

void SDLWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStart = event->pos();
    emit dragStarted();
}

void SDLWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event->modifiers() == Qt::ShiftModifier && m_producer) {
        emit seekTo(m_producer->get_length() * event->x() / width());
        return;
    }
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - m_dragStart).manhattanLength() < QApplication::startDragDistance())
        return;
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/mlt+xml", "");
    drag->setMimeData(mimeData);
    drag->exec(Qt::LinkAction);
}

// MLT consumer-frame-show event handler
void SDLWidget::on_frame_show(mlt_consumer, void* self, mlt_frame frame_ptr)
{
    SDLWidget* widget = static_cast<SDLWidget*>(self);
    Frame frame(frame_ptr);
    emit widget->frameReceived(Mlt::QFrame(frame));
}

