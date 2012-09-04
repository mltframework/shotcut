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
#include <QVariant>

using namespace Mlt;

SDLWidget::SDLWidget(QWidget *parent)
    : QWidget(parent)
    , Controller()
{
    // Required for SDL embeddding
    parent->setAttribute(Qt::WA_NativeWindow);
}

int SDLWidget::open(Mlt::Producer* producer, bool isMulti)
{
    int error = Controller::open(producer, isMulti);

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
#if defined(Q_WS_WIN)
            // sdl_preview does not work good on Windows
            serviceName = "sdl";
#else
            serviceName = "sdl_preview";
#endif
        if (isMulti)
            m_consumer = new Mlt::FilteredConsumer(profile(), "multi");
        else
            m_consumer = new Mlt::FilteredConsumer(profile(), serviceName.toAscii().constData());
    }
    if (m_consumer->is_valid()) {
        // Connect the producer to the consumer - tell it to "run" later
        m_consumer->connect(*m_producer);
        // Make an event handler for when a frame's image should be displayed
        m_consumer->listen("consumer-frame-show", this, (mlt_listener) on_frame_show);
        m_consumer->set("real_time", property("realtime").toBool()? 1 : -1);

        if (isMulti) {
            m_consumer->set("terminate_on_pause", 0);
            // Embed the SDL window in our GUI.
            m_consumer->set("0.window_id", (int) this->winId());
            // Set the background color
            m_consumer->set("0.window_background", palette().color(QPalette::Window).name().toAscii().constData());
            if (!profile().progressive())
                m_consumer->set("progressive", property("progressive").toBool());
            m_consumer->set("0.rescale", property("rescale").toString().toAscii().constData());
            m_consumer->set("0.deinterlace_method", property("deinterlace_method").toString().toAscii().constData());
#ifndef Q_WS_WIN
            m_consumer->set("0.scrub_audio", 1);
#endif
        }
        else {
            // Embed the SDL window in our GUI.
            m_consumer->set("window_id", (int) this->winId());
            // Set the background color
            m_consumer->set("window_background", palette().color(QPalette::Window).name().toAscii().constData());
            if (!profile().progressive())
                m_consumer->set("progressive", property("progressive").toBool());
            m_consumer->set("rescale", property("rescale").toString().toAscii().constData());
            m_consumer->set("deinterlace_method", property("deinterlace_method").toString().toAscii().constData());
#ifndef Q_WS_WIN
            m_consumer->set("scrub_audio", 1);
#endif
        }
    }
    else {
        // Cleanup on error
        error = 2;
        Controller::close();
    }
    return error;
}

// MLT consumer-frame-show event handler
void SDLWidget::on_frame_show(mlt_consumer, void* self, mlt_frame frame_ptr)
{
    SDLWidget* widget = static_cast<SDLWidget*>(self);
    Frame frame(frame_ptr);
    emit widget->frameReceived(Mlt::QFrame(frame));
}

