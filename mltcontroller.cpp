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
#include <QMetaType>
#include <QSettings>
#include <Mlt.h>
#include "glwidget.h"
#include "sdlwidget.h"

namespace Mlt {

QFrame::QFrame(QObject *parent)
    : QObject(parent)
    , m_frame(0)
{}

QFrame::QFrame(const Frame& frame)
    : QObject(0)
{
    Frame* f = const_cast<Frame*>(&frame);
    m_frame = new Frame(f->get_frame());
}

QFrame::QFrame(const QFrame& qframe)
    : QObject(0)
{
    Frame* frame = qframe.frame();
    m_frame = new Frame(frame->get_frame());
}

QFrame::~QFrame() {
    delete m_frame;
}

Frame* QFrame::frame() const {
    return m_frame;
}

QImage QFrame::image()
{
    if (m_frame) {
        int width = 0;
        int height = 0;
        // TODO: change the format if using a pixel shader
        mlt_image_format format = mlt_image_rgb24a;
        const uint8_t* image = m_frame->get_image(format, width, height);
        QImage qimage(width, height, QImage::Format_ARGB32);
        memcpy(qimage.scanLine(0), image, width * height * 4);
        return qimage;
    }
    else {
        return QImage();
    }
}

Controller::Controller()
    : m_repo(Mlt::Factory::init())
    , m_producer(0)
    , m_consumer(0)
    , m_profile(new Mlt::Profile)
    , m_volumeFilter(0)
    , m_jackFilter(0)
{
}

Controller& Controller::singleton(QWidget* parent)
{
    static Controller* instance = 0;
    if (!instance) {
        qRegisterMetaType<QFrame>("Mlt::QFrame");
#if defined(Q_WS_MAC) || defined(Q_WS_WIN)
        instance = new GLWidget(parent);
#else
        QSettings settings;
        if (settings.value("player/opengl", true).toBool())
            instance = new GLWidget(parent);
        else
            instance = new SDLWidget(parent);
#endif
    }
    return *instance;
}

Controller::~Controller()
{
    close();
    delete m_consumer;
    m_consumer = 0;
    delete m_volumeFilter;
    m_volumeFilter = 0;
    delete m_jackFilter;
    m_jackFilter = 0;
    delete m_profile;
    // TODO: this is commented out because it causes crash on closing queued QFrames.
//    Mlt::Factory::close();
}

int Controller::open(Mlt::Producer* producer, bool)
{
    int error = 0;

    if (producer != m_producer)
        close();
    if (producer && producer->is_valid()) {
        m_producer = producer;
    }
    else {
        // Cleanup on error
        error = 1;
        delete producer;
    }
    return error;
}

int Controller::open(const char* url)
{
    int error = 0;

    close();
    m_producer = new Mlt::Producer(profile(), url);
    if (m_producer->is_valid()) {
        double fps = profile().fps();
        if (!profile().get_profile()->is_explicit)
            profile().from_producer(*m_producer);
        if (profile().fps() != fps) {
            // reopen with the correct fps
            delete m_producer;
            m_producer = new Mlt::Producer(profile(), url);
        }
    }
    else {
        delete m_producer;
        m_producer = 0;
        error = 1;
    }
    return error;
}

void Controller::close()
{
    if (m_consumer)
        m_consumer->stop();
    delete m_producer;
    m_producer = 0;
}

void Controller::play(double speed)
{
    if (m_producer)
        m_producer->set_speed(speed);
    // If we are paused, then we need to "unlock" sdl_still.
    if (m_consumer) {
        m_consumer->start();
        refreshConsumer();
    }
    if (m_jackFilter)
        m_jackFilter->fire_event("jack-start");
}

void Controller::pause()
{
    if (m_producer)
        m_producer->pause();
    if (m_jackFilter)
        m_jackFilter->fire_event("jack-stop");
}

void Controller::stop()
{
    if (m_consumer && !m_consumer->is_stopped())
        m_consumer->stop();
    if (m_producer)
        m_producer->seek(0);
    if (m_jackFilter)
        m_jackFilter->fire_event("jack-stop");
}

void Controller::on_jack_started(mlt_properties, void* object, mlt_position *position)
{
    if (object && position)
        ((Controller*) object)->onJackStarted(*position);
}

void Controller::onJackStarted(int position)
{
    if (m_producer) {
        m_producer->set_speed(1);
        m_producer->seek(position);
        refreshConsumer();
    }
}

void Controller::on_jack_stopped(mlt_properties, void* object, mlt_position *position)
{
    if (object && position)
        ((Controller*) object)->onJackStopped(*position);
}

void Controller::onJackStopped(int position)
{
    if (m_producer) {
        m_producer->pause();
        m_producer->seek(position);
    }
    if (m_consumer)
        m_consumer->purge();
    refreshConsumer();
}

bool Controller::enableJack(bool enable)
{
	if (!m_consumer)
		return true;
	if (enable && !m_jackFilter) {
		m_jackFilter = new Mlt::Filter(profile(), "jackrack");
		if (m_jackFilter->is_valid()) {
			m_consumer->attach(*m_jackFilter);
			m_consumer->set("audio_off", 1);
			if (m_producer && m_producer->get_int("seekable")) {
				m_jackFilter->listen("jack-started", this, (mlt_listener) on_jack_started);
				m_jackFilter->listen("jack-stopped", this, (mlt_listener) on_jack_stopped);
			}
		}
		else {
			delete m_jackFilter;
			m_jackFilter = 0;
			return false;
		}
	}
	else if (!enable && m_jackFilter) {
		m_consumer->detach(*m_jackFilter);
		delete m_jackFilter;
		m_jackFilter = 0;
		m_consumer->set("audio_off", 0);
		m_consumer->stop();
		m_consumer->start();
	}
	return true;
}


void Controller::setVolume(double volume)
{
    if (m_consumer) {
        if (!m_volumeFilter) {
            m_volumeFilter = new Filter(profile(), "volume");
            m_consumer->attach(*m_volumeFilter);
        }
        m_volumeFilter->set("gain", volume);
    }
}

double Controller::volume() const
{
    return m_volumeFilter? m_volumeFilter->get_double("gain") : 1.0;
}

void Controller::onWindowResize()
{
    refreshConsumer();
}

void Controller::seek(int position)
{
    if (m_producer)
        m_producer->seek(position);
    if (m_consumer)
        m_consumer->purge();
    if (m_jackFilter)
        mlt_events_fire(m_jackFilter->get_properties(), "jack-seek", &position, NULL);
    refreshConsumer();
}

void Controller::refreshConsumer()
{
    if (m_consumer) // need to refresh consumer when paused
        m_consumer->set("refresh", 1);
}

QString Controller::saveXML(const QString& filename)
{
    Mlt::Consumer c(profile(), "xml", filename.toUtf8().constData());
    Mlt::Service s(m_producer->get_service());
    int ignore = s.get_int("ignore_points");
    if (ignore)
        s.set("ignore_points", 0);
    c.connect(s);
    c.start();
    s.set("ignore_points", ignore);
    return QString::fromUtf8(c.get(filename.toUtf8().constData()));
}

int Controller::consumerChanged()
{
    int error = 0;
    double gain = volume();

    if (m_consumer) {
        bool jackEnabled = m_jackFilter != 0;
        m_consumer->stop();
        delete m_consumer;
        m_consumer = 0;
        delete m_volumeFilter;
        m_volumeFilter = 0;
        delete m_jackFilter;
        m_jackFilter= 0;
        error = reconfigure(false);
        if (m_consumer) {
            enableJack(jackEnabled);
            setVolume(gain);
            m_consumer->start();
        }
    }
    return error;
}

int Controller::setProfile(const QString& profile_name)
{
    int error = 0;
    bool reopen = m_consumer != 0;
    double speed = m_producer? m_producer->get_speed(): 0;
    const char* position = m_producer? m_producer->frame_time() : 0;
    double gain = m_volumeFilter? m_volumeFilter->get_double("gain") : 1.0;
    bool jackEnabled = m_jackFilter != 0;

    if (m_consumer)
        m_consumer->stop();
    delete m_consumer;
    m_consumer = 0;
    delete m_volumeFilter;
    m_volumeFilter = 0;
    delete m_jackFilter;
    m_jackFilter= 0;
    delete m_profile;

    m_profile = new Mlt::Profile(profile_name.toAscii().constData());
    if (!profile_name.isEmpty())
        m_profile->set_explicit(1);

    if (reopen) {
        Mlt::Consumer c(profile(), "xml", "xml-string");
        Mlt::Service s(m_producer->get_service());
        c.connect(s);
        c.start();
        m_producer = new Mlt::Producer(profile(), "xml-string", c.get("xml-string"));
        this->open(m_producer);
        enableJack(jackEnabled);
        setVolume(gain);
        if (m_producer)
            m_producer->seek(position);
        play(speed);
    }

    return error;
}

QString Controller::resource() const
{
    QString resource;
    if (!m_producer)
        return resource;
    resource = QString(m_producer->get("resource"));
    if (resource == "<tractor>") {
        Mlt::Tractor tractor((mlt_tractor) m_producer->get_service());
        Mlt::Multitrack* multitrack = tractor.multitrack();
        if (multitrack->is_valid()) {
            Mlt::Producer* producer = multitrack->track(0);
            if (producer->is_valid())
                resource = QString(producer->get("resource"));
            delete producer;
        }
        delete multitrack;
    }
    return resource;
}

} // namespace
