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

#include "mltcontroller.h"
#include <QWidget>
#include <QPalette>
#include <Mlt.h>
#include "glwidget.h"
#include "sdlwidget.h"

namespace Mlt {

Controller::Controller()
    : m_profile(0)
    , m_producer(0)
    , m_consumer(0)
{
}

Controller* Controller::createWidget(QWidget* parent)
{
    Mlt::Factory::init();
#ifdef Q_WS_MAC
    return new GLWidget(parent);
#else
    return new SDLWidget(parent);
#endif
}

Controller::~Controller()
{
    close();
    Mlt::Factory::close();
}

int Controller::open(const char* url, const char* profile)
{
    int error = 0;

    close();
    m_profile = new Mlt::Profile(profile);
    m_producer = new Mlt::Producer(*m_profile, url);
    if (!m_producer->is_valid()) {
        // Cleanup on error
        error = 1;
        delete m_producer;
        m_producer = 0;
        delete m_profile;
        m_profile = 0;
    }
    else {
        if (!profile)
            // Automate profile
            m_profile->from_producer(*m_producer);
    }
    return error;
}

void Controller::close()
{
    if (m_consumer)
        m_consumer->stop();
    delete m_consumer;
    m_consumer = 0;
    delete m_producer;
    m_producer = 0;
    delete m_profile;
    m_profile = 0;
}

void Controller::play()
{
    if (m_producer)
        m_producer->set_speed(1);
    // If we are paused, then we need to "unlock" sdl_still.
    if (m_consumer)
        m_consumer->set("refresh", 1);
}

void Controller::pause()
{
    if (m_producer)
        m_producer->pause();
}

void Controller::setVolume(double volume)
{
    if (m_consumer)
        m_consumer->set("volume", volume);
}

QImage Controller::getImage(void* frame_ptr)
{
    Mlt::Frame* frame = static_cast<Mlt::Frame*>(frame_ptr);
    int width = 0;
    int height = 0;
    // TODO: change the format if using a pixel shader
    mlt_image_format format = mlt_image_rgb24a;
    const uint8_t* image = frame->get_image(format, width, height);
    QImage qimage(width, height, QImage::Format_ARGB32);
    memcpy(qimage.scanLine(0), image, width * height * 4);
    return qimage;
}

void Controller::onWindowResize()
{
    if (m_consumer)
        // When paused this tells sdl_still to update.
        m_consumer->set("refresh", 1);
}

} // namespace
