/*
 * Copyright (c) 2011 Dan Dennedy <dan@dennedy.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "mltcontroller.h"
#include <QWidget>
#include <QPalette>

MltController::MltController(QObject *parent)
    : QObject (parent)
    , m_profile (0)
    , m_producer (0)
    , m_consumer (0)
{
}

MltController::~MltController ()
{
    close();
    Mlt::Factory::close();
}

void MltController::init ()
{
    Mlt::Factory::init();
}

int MltController::open (const char* url, const char* profile)
{
    int error = 0;

    close ();
    m_profile = new Mlt::Profile (profile);
    m_producer = new Mlt::Producer (*m_profile, url);
    if (!m_producer->is_valid ()) {
        // Cleanup on error
        error = 1;
        delete m_producer;
        m_producer = 0;
        delete m_profile;
        m_profile = 0;
    }
    else {
//        if ( !profile )
//            // Automate profile
//            m_profile->from_producer (*m_producer);
#ifdef Q_WS_MAC
        // use SDL for audio, OpenGL for video
        m_consumer = new Mlt::Consumer (*m_profile, "sdl_audio");
#elif defined(Q_WS_WIN)
        // sdl_preview does not work good on Windows
        m_consumer = new Mlt::Consumer (*m_profile, "sdl");
#else
        m_consumer = new Mlt::Consumer (*m_profile, "sdl_preview");
#endif
        if (m_consumer->is_valid ()) {
            // Embed the SDL window in our GUI.
            QWidget* widget = qobject_cast<QWidget*> (parent());
            m_consumer->set ("window_id", (int) widget->winId());

#ifndef Q_WS_WIN
            // Set the background color
            // XXX: Incorrect color on Windows
            QPalette pal;
            m_consumer->set ("window_background", pal.color (QPalette::Window).name().toAscii().constData());
#endif

            // Connect the producer to the consumer - tell it to "run" later
            m_consumer->connect (*m_producer);
            // Make an event handler for when a frame's image should be displayed
            m_consumer->listen ("consumer-frame-show", this, (mlt_listener) on_frame_show);
            m_consumer->start ();
        }
        else {
            // Cleanup on error
            error = 2;
            delete m_consumer;
            m_consumer = 0;
            delete m_producer;
            m_producer = 0;
            delete m_profile;
            m_profile = 0;
        }
    }
    return error;
}

void MltController::close ()
{
    if (m_consumer)
        m_consumer->stop ();
    delete m_consumer;
    m_consumer = 0;
    delete m_producer;
    m_producer = 0;
    delete m_profile;
    m_profile = 0;
}

void MltController::play ()
{
    if (m_producer)
        m_producer->set_speed (1);
    // If we are paused, then we need to "unlock" sdl_still.
    if (m_consumer)
        m_consumer->set ("refresh", 1);
}

void MltController::pause ()
{
    if (m_producer)
        m_producer->pause ();
}

void MltController::setVolume (double volume)
{
    if (m_consumer)
        m_consumer->set ("volume", volume);
}

QImage MltController::getImage (void* frame_ptr)
{
    Mlt::Frame* frame = static_cast<Mlt::Frame*> (frame_ptr);
    int width = 0;
    int height = 0;
    // TODO: change the format if using a pixel shader
    mlt_image_format format = mlt_image_rgb24a;
    const uint8_t* image = frame->get_image (format, width, height);
    delete frame;
    QImage qimage (width, height, QImage::Format_ARGB32);
    memcpy (qimage.scanLine(0), image, width * height * 4);
    return qimage;
}

void MltController::onWindowResize ()
{
    if (m_consumer)
        // When paused this tells sdl_still to update.
        m_consumer->set ("refresh", 1);
}

// MLT consumer-frame-show event handler
void MltController::on_frame_show (mlt_consumer, void* self, mlt_frame frame_ptr)
{
    MltController* controller = static_cast<MltController*> (self);
    Mlt::Frame* frame = new Mlt::Frame (frame_ptr);
    emit controller->frameReceived (frame, (unsigned) mlt_frame_get_position (frame_ptr));
}
