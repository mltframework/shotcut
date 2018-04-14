/*
 * Copyright (c) 2011-2018 Meltytech, LLC
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
#include <QFileInfo>
#include <QUuid>
#include <Logger.h>
#include <Mlt.h>
#include "glwidget.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "mainwindow.h"

namespace Mlt {

static const int kThumbnailOutSeekFactor = 5;

static Controller* instance = 0;
const QString XmlMimeType("application/mlt+xml");

static int alignWidth(int width)
{
    return (width + 7) / 8 * 8;
}

Controller::Controller()
    : m_producer(0)
    , m_consumer(0)
    , m_audioChannels(2)
    , m_jackFilter(0)
    , m_volume(1.0)
    , m_skipJackEvents(0)
{
    LOG_DEBUG() << "begin";
    m_repo = Mlt::Factory::init();
    m_profile = new Mlt::Profile("atsc_1080p_25");
    m_filtersClipboard.reset(new Mlt::Producer(profile(), "color", "black"));
    updateAvformatCaching(0);
    LOG_DEBUG() << "end";
}

Controller& Controller::singleton(QObject *parent)
{
    if (!instance) {
        qRegisterMetaType<Mlt::Frame>("Mlt::Frame");
        qRegisterMetaType<SharedFrame>("SharedFrame");
        instance = new GLWidget(parent);
    }
    return *instance;
}

Controller::~Controller()
{
    close();
    closeConsumer();
    delete m_profile;
}

void Controller::destroy()
{
    delete instance;
}

int Controller::setProducer(Mlt::Producer* producer, bool)
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

int Controller::open(const QString &url)
{
    int error = 0;

    close();

    if (Settings.playerGPU() && !profile().is_explicit())
        // Prevent loading normalizing filters, which might be Movit ones that
        // may not have a proper OpenGL context when requesting a sample frame.
        m_producer = new Mlt::Producer(profile(), "abnormal", url.toUtf8().constData());
    else
        m_producer = new Mlt::Producer(profile(), url.toUtf8().constData());
    if (m_producer->is_valid()) {
        double fps = profile().fps();
        if (!profile().is_explicit()) {
            profile().from_producer(*m_producer);
            profile().set_width(alignWidth(profile().width()));
        }
        if ( url.endsWith(".mlt") ) {
            // Load the number of audio channels being used when this project was created.
            int channels = m_producer->get_int(kShotcutProjectAudioChannels);
            if (!channels)
                channels = 2;
            m_audioChannels = channels;
        }
        if (profile().fps() != fps || (Settings.playerGPU() && !profile().is_explicit())) {
            // Reload with correct FPS or with Movit normalizing filters attached.
            delete m_producer;
            m_producer = new Mlt::Producer(profile(), url.toUtf8().constData());
        }
        // Convert avformat to avformat-novalidate so that XML loads faster.
        if (!qstrcmp(m_producer->get("mlt_service"), "avformat")) {
            m_producer->set("mlt_service", "avformat-novalidate");
            m_producer->set("mute_on_pause", 0);
        }
        if (m_url.isEmpty() && QString(m_producer->get("xml")) == "was here") {
            if (m_producer->get_int("_original_type") != tractor_type ||
               (m_producer->get_int("_original_type") == tractor_type && m_producer->get("shotcut")))
                m_url = url;
        }
        setImageDurationFromDefault(m_producer);
    }
    else {
        delete m_producer;
        m_producer = 0;
        error = 1;
    }
    return error;
}

bool Controller::openXML(const QString &filename)
{
    bool error = true;
    close();
    Producer* producer = new Mlt::Producer(profile(), "xml", filename.toUtf8().constData());
    if (producer->is_valid()) {
        double fps = profile().fps();
        if (!profile().is_explicit()) {
            profile().from_producer(*producer);
            profile().set_width(alignWidth(profile().width()));
        }
        if (profile().fps() != fps) {
            // reopen with the correct fps
            delete producer;
            producer = new Mlt::Producer(profile(), "xml", filename.toUtf8().constData());
        }
        producer->set(kShotcutVirtualClip, 1);
        producer->set("resource", filename.toUtf8().constData());
        setProducer(new Producer(producer));
        error = false;
    }
    delete producer;
    return error;
}

void Controller::close()
{
    if (m_profile->is_explicit()) {
        pause();
    } else if (m_consumer && !m_consumer->is_stopped()) {
        m_consumer->stop();
    }
    if (isSeekableClip()) {
        setSavedProducer(m_producer);
    }
    delete m_producer;
    m_producer = 0;
}

void Controller::closeConsumer()
{
    if (m_consumer)
        m_consumer->stop();
    delete m_consumer;
    m_consumer = 0;
    delete m_jackFilter;
    m_jackFilter = 0;
}

void Controller::play(double speed)
{
    if (m_jackFilter) {
        if (speed == 1.0)
            m_jackFilter->fire_event("jack-start");
        else
            stopJack();
    }
    if (m_producer)
        m_producer->set_speed(speed);
    if (m_consumer) {
        // Restore real_time behavior and work-ahead buffering
        if (!Settings.playerGPU())
        if (m_consumer->get_int("real_time") != realTime()) {
            m_consumer->set("real_time", realTime());
            m_consumer->set("buffer", 25);
            m_consumer->set("prefill", 1);
            // Changes to real_time require a consumer restart if running.
            if (!m_consumer->is_stopped())
                m_consumer->stop();
        }
        m_consumer->start();
        refreshConsumer(Settings.playerScrubAudio());
    }
    setVolume(m_volume);
}

void Controller::pause()
{
    if (m_producer && m_producer->get_speed() != 0) {
        if (!Settings.playerGPU())
        if (m_consumer && m_consumer->is_valid()) {
            // Disable real_time behavior and buffering for frame accurate seeking.
            m_consumer->set("real_time", -1);
            m_consumer->set("buffer", 0);
            m_consumer->set("prefill", 0);
        }
        m_producer->set_speed(0);
        m_producer->seek(m_consumer->position() + 1);
        if (m_consumer && m_consumer->is_valid()) {
            m_consumer->purge();
            m_consumer->start();
        }
    }
    if (m_jackFilter) {
        stopJack();
        int position = m_producer->position();
        ++m_skipJackEvents;
        mlt_events_fire(m_jackFilter->get_properties(), "jack-seek", &position, NULL);
    }
    setVolume(m_volume);
}

void Controller::stop()
{
    if (m_consumer && !m_consumer->is_stopped())
        m_consumer->stop();
    if (m_producer)
        m_producer->seek(0);
    stopJack();
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
    if (m_skipJackEvents) {
        --m_skipJackEvents;
    } else {
        if (m_producer) {
            if (m_producer->get_speed() != 0) {
                Event *event = m_consumer->setup_wait_for("consumer-sdl-paused");
                int result = m_producer->set_speed(0);
                if (result == 0 && m_consumer->is_valid() && !m_consumer->is_stopped())
                    m_consumer->wait_for(event);
                delete event;
            }
            m_producer->seek(position);
        }
        if (m_consumer && m_consumer->get_int("real_time") >= -1)
            m_consumer->purge();
        refreshConsumer();
    }
}

void Controller::stopJack()
{
    if (m_jackFilter) {
        m_skipJackEvents = 2;
        m_jackFilter->fire_event("jack-stop");
    }
}

bool Controller::enableJack(bool enable)
{
	if (!m_consumer)
		return true;
	if (enable && !m_jackFilter) {
		m_jackFilter = new Mlt::Filter(profile(), "jack", "Shotcut player");
		if (m_jackFilter->is_valid()) {
            m_jackFilter->set("channels", Settings.playerAudioChannels());
            switch (Settings.playerAudioChannels()) {
            case 8:
                m_jackFilter->set("in_8", "-");
                m_jackFilter->set("out_8", "system:playback_8");
            case 7:
                m_jackFilter->set("in_7", "-");
                m_jackFilter->set("out_7", "system:playback_7");
            case 6:
                m_jackFilter->set("in_6", "-");
                m_jackFilter->set("out_6", "system:playback_6");
            case 5:
                m_jackFilter->set("in_5", "-");
                m_jackFilter->set("out_5", "system:playback_5");
            case 4:
                m_jackFilter->set("in_4", "-");
                m_jackFilter->set("out_4", "system:playback_4");
            case 3:
                m_jackFilter->set("in_3", "-");
                m_jackFilter->set("out_3", "system:playback_3");
            case 2:
                m_jackFilter->set("in_2", "-");
                m_jackFilter->set("out_2", "system:playback_2");
            case 1:
                m_jackFilter->set("in_1", "-");
                m_jackFilter->set("out_1", "system:playback_1");
            default:
                break;
            }
			m_consumer->attach(*m_jackFilter);
			m_consumer->set("audio_off", 1);
			if (isSeekable()) {
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


void Controller::setVolume(double volume, bool muteOnPause)
{
    m_volume = volume;

    // Keep the consumer muted when paused
    if (muteOnPause && m_producer && m_producer->get_speed() == 0) {
        volume = 0.0;
    }

    if (m_consumer) {
        if (m_consumer->get("mlt_service") == QString("multi")) {
            m_consumer->set("0.volume", volume);
        } else {
            m_consumer->set("volume", volume);
        }
    }

}

double Controller::volume() const
{
    return m_volume;
}

void Controller::onWindowResize()
{
    refreshConsumer();
}

void Controller::seek(int position)
{
    setVolume(m_volume, false);
    if (m_producer) {
        // Always pause before seeking (if not already paused).
        if (!Settings.playerGPU())
        if (m_consumer && m_consumer->is_valid() && m_producer->get_speed() != 0) {
            // Disable real_time behavior and buffering for frame accurate seeking.
            m_consumer->set("real_time", -1);
            m_consumer->set("buffer", 0);
            m_consumer->set("prefill", 0);
        }
        m_producer->set_speed(0);
        m_producer->seek(position);
        if (m_consumer && m_consumer->is_valid()) {
            if (m_consumer->is_stopped()) {
                m_consumer->start();
            } else {
                m_consumer->purge();
                refreshConsumer(Settings.playerScrubAudio());
            }
        }
    }
    if (m_jackFilter) {
        stopJack();
        ++m_skipJackEvents;
        mlt_events_fire(m_jackFilter->get_properties(), "jack-seek", &position, NULL);
    }
}

void Controller::refreshConsumer(bool scrubAudio)
{
    if (m_consumer) {
        // need to refresh consumer when paused
        m_consumer->set("scrub_audio", scrubAudio);
        m_consumer->set("refresh", 1);
    }
}

void Controller::saveXML(const QString& filename, Service* service, bool withRelativePaths)
{
    Consumer c(profile(), "xml", filename.toUtf8().constData());
    Service s(service? service->get_service() : m_producer->get_service());
    if (s.is_valid()) {
        s.set(kShotcutProjectAudioChannels, m_audioChannels);
        int ignore = s.get_int("ignore_points");
        if (ignore)
            s.set("ignore_points", 0);
        c.set("time_format", "clock");
        c.set("no_meta", 1);
        c.set("store", "shotcut");
        if (withRelativePaths) {
            c.set("root", QFileInfo(filename).absolutePath().toUtf8().constData());
            c.set("no_root", 1);
        }
        c.set("title", QString("Shotcut version ").append(SHOTCUT_VERSION).toUtf8().constData());
        c.connect(s);
        c.start();
        if (ignore)
            s.set("ignore_points", ignore);
    }
}

QString Controller::XML(Service* service, bool withProfile)
{
    static const char* propertyName = "string";
    Consumer c(profile(), "xml", propertyName);
    Service s(service? service->get_service() : m_producer->get_service());
    if (!s.is_valid())
        return "";
    int ignore = s.get_int("ignore_points");
    if (ignore)
        s.set("ignore_points", 0);
    c.set("no_meta", 1);
    c.set("no_profile", !withProfile);
    c.set("store", "shotcut");
    c.connect(s);
    c.start();
    if (ignore)
        s.set("ignore_points", ignore);
    return QString::fromUtf8(c.get(propertyName));
}

int Controller::consumerChanged()
{
    int error = 0;
    if (m_consumer) {
        bool jackEnabled = m_jackFilter != 0;
        m_consumer->stop();
        delete m_consumer;
        m_consumer = 0;
        delete m_jackFilter;
        m_jackFilter= 0;
        error = reconfigure(false);
        if (m_consumer) {
            enableJack(jackEnabled);
            setVolume(m_volume);
            m_consumer->start();
        }
    }
    return error;
}

void Controller::setProfile(const QString& profile_name)
{
    LOG_DEBUG() << "setting to profile" << (profile_name.isEmpty()? "Automatic" : profile_name);
    if (!profile_name.isEmpty()) {
        Mlt::Profile tmp(profile_name.toLatin1().constData());
        m_profile->set_colorspace(tmp.colorspace());
        m_profile->set_frame_rate(tmp.frame_rate_num(), tmp.frame_rate_den());
        m_profile->set_height(tmp.height());
        m_profile->set_progressive(tmp.progressive());
        m_profile->set_sample_aspect(tmp.sample_aspect_num(), tmp.sample_aspect_den());
        m_profile->set_display_aspect(tmp.display_aspect_num(), tmp.display_aspect_den());
        m_profile->set_width(alignWidth(tmp.width()));
        m_profile->set_explicit(true);
    } else {
        m_profile->set_explicit(false);
        if (m_producer) {
            m_profile->from_producer(*m_producer);
            m_profile->set_width(alignWidth(m_profile->width()));
        }
    }
}

void Controller::setAudioChannels(int audioChannels)
{
    LOG_DEBUG() << audioChannels;
    if (audioChannels != m_audioChannels) {
        m_audioChannels = audioChannels;
        restart();
    }
}

QString Controller::resource() const
{
    QString resource;
    if (!m_producer)
        return resource;
    resource = QString(m_producer->get("resource"));
    return resource;
}

bool Controller::isSeekable(Producer* p) const
{
    bool seekable = false;
    Mlt::Producer* producer = p? p : m_producer;
    if (producer && producer->is_valid()) {
        if (producer->get("force_seekable")) {
            seekable = producer->get_int("force_seekable");
        } else {
            seekable = producer->get_int("seekable");
            if (!seekable && producer->get("mlt_type")) {
                // XXX what was this for?
                seekable = !strcmp(producer->get("mlt_type"), "mlt_producer");
            }
            if (!seekable) {
                // These generators can take an out point to define their length.
                // TODO: Currently, these max out at 15000 frames, which is arbitrary.
                QString service(producer->get("mlt_service"));
                seekable = (service == "color") || service.startsWith("frei0r.") || (service =="tone") || (service =="count");
            }
        }
    }
    return seekable;
}

bool Controller::isClip() const
{
    return !isPlaylist() && !isMultitrack();
}

bool Controller::isSeekableClip()
{
    return isClip() && isSeekable();
}

bool Controller::isPlaylist() const
{
    return m_producer && m_producer->is_valid() &&
          !m_producer->get_int(kShotcutVirtualClip) &&
            (m_producer->get_int("_original_type") == playlist_type || resource() == "<playlist>");
}

bool Controller::isMultitrack() const
{
    return m_producer && m_producer->is_valid()
        && !m_producer->get_int(kShotcutVirtualClip)
        && (m_producer->get_int("_original_type") == tractor_type || resource() == "<tractor>")
            && (m_producer->get("shotcut"));
}

bool Controller::isImageProducer(Service* service) const
{
    if (service && service->is_valid()) {
        QString serviceName = service->get("mlt_service");
        return (serviceName == "pixbuf" || serviceName == "qimage");
    }
    return false;
}

void Controller::rewind()
{
    if (!m_producer || !m_producer->is_valid())
        return;
    // Starting the consumer when producer at its end fails. So, first seek to
    // frame before last.
    if (m_producer->position() >= m_producer->get_length() - 1)
        m_producer->seek(m_producer->get_length() - 2);
    if (m_producer->get_speed() >= 0) {
        play(-1.0);
    } else {
        stopJack();
        m_producer->set_speed(m_producer->get_speed() * 2);
    }
}

void Controller::fastForward()
{
    if (!m_producer || !m_producer->is_valid())
        return;
    if (m_producer->get_speed() <= 0) {
        play();
    } else {
        stopJack();
        m_producer->set_speed(m_producer->get_speed() * 2);
    }
}

void Controller::previous(int currentPosition)
{
    if (isMultitrack()) return;
    if (currentPosition > m_producer->get_out())
        seek(MLT.producer()->get_out());
    else if (currentPosition <= m_producer->get_in())
        seek(0);
    else
        seek(m_producer->get_in());
}

void Controller::next(int currentPosition)
{
    if (isMultitrack()) return;
    if (currentPosition < m_producer->get_in())
        seek(m_producer->get_in());
    else if (currentPosition >= m_producer->get_out())
        seek(m_producer->get_length() - 1);
    else
        seek(m_producer->get_out());
}

void Controller::setIn(int in)
{
    if (m_producer && m_producer->is_valid()) {
        m_producer->set("in", in);

        // Adjust all filters that have an explicit duration.
        int n = m_producer->filter_count();
        for (int i = 0; i < n; i++) {
            Filter* filter = m_producer->filter(i);
            if (filter && filter->is_valid() && filter->get_length() > 0) {
                if (QString(filter->get(kShotcutFilterProperty)).startsWith("fadeIn")
                        || QString(filter->get("mlt_service")) == "webvfx") {
                    filter->set_in_and_out(in, in + filter->get_length() - 1);
                }
            }
            delete filter;
        }
    }
}

void Controller::setOut(int out)
{
    if (m_producer && m_producer->is_valid()) {
        m_producer->set("out", out);

        // Adjust all filters that have an explicit duration.
        int n = m_producer->filter_count();
        for (int i = 0; i < n; i++) {
            Filter* filter = m_producer->filter(i);
            if (filter && filter->is_valid() && filter->get_length() > 0) {
                if (QString(filter->get(kShotcutFilterProperty)).startsWith("fadeOut")
                        || QString(filter->get("mlt_service")) == "webvfx") {
                    int in = out - filter->get_length() + 1;
                    filter->set_in_and_out(in, out);
                }
            }
            delete filter;
        }
    }
}

void Controller::restart()
{
    if (!m_consumer || !m_consumer->is_valid() || !m_producer || !m_producer->is_valid())
        return;
    if (m_producer->get_speed() != 0) {
        // Update the real_time property if not paused.
        m_consumer->set("real_time", realTime());
    }
    const char* position = m_consumer->frames_to_time(m_consumer->position());
    double speed = m_producer->get_speed();
    QString xml = XML();
    stop();
    if (!setProducer(new Mlt::Producer(profile(), "xml-string", xml.toUtf8().constData()))) {
#ifdef Q_OS_WIN
        play(speed);
        if (m_producer && m_producer->is_valid())
            m_producer->seek(position);
        // Windows needs an extra kick here when not paused!
        if (speed != 0.0)
            play(speed);
#else
        if (m_producer && m_producer->is_valid())
            m_producer->seek(position);
        play(speed);
#endif
    }
}

void Controller::resetURL()
{
    m_url = QString();
}

QImage Controller::image(Mlt::Frame* frame, int width, int height)
{
    QImage result;
    if (frame && frame->is_valid()) {
        if (width > 0 && height > 0) {
            frame->set("rescale.interp", "bilinear");
            frame->set("deinterlace_method", "onefield");
            frame->set("top_field_first", -1);
        }
        mlt_image_format format = mlt_image_rgb24a;
        const uchar *image = frame->get_image(format, width, height);
        if (image) {
            QImage temp(width, height, QImage::Format_ARGB32);
            memcpy(temp.scanLine(0), image, width * height * 4);
            result = temp.rgbSwapped();
        }
    } else {
        result = QImage(width, height, QImage::Format_ARGB32);
        result.fill(QColor(Qt::red).rgb());
    }
    return result;
}

QImage Controller::image(Producer& producer, int frameNumber, int width, int height)
{
    QImage result;
    if (frameNumber > producer.get_length() - kThumbnailOutSeekFactor) {
        producer.seek(frameNumber - kThumbnailOutSeekFactor - 1);
        for (int i = 0; i < kThumbnailOutSeekFactor; ++i) {
            QScopedPointer<Mlt::Frame> frame(producer.get_frame());
            QImage temp = image(frame.data(), width, height);
            if (!temp.isNull())
                result = temp;
        }
    } else {
        producer.seek(frameNumber);
        QScopedPointer<Mlt::Frame> frame(producer.get_frame());
        result = image(frame.data(), width, height);
    }
    return result;
}

void Controller::updateAvformatCaching(int trackCount)
{
    int i = QThread::idealThreadCount() + trackCount;
    mlt_service_cache_set_size(NULL, "producer_avformat", qMax(4, i));
}

bool Controller::isAudioFilter(const QString &name)
{
    QScopedPointer<Properties> metadata(m_repo->metadata(filter_type, name.toLatin1().constData()));
    if (metadata->is_valid()) {
        Properties tags(metadata->get_data("tags"));
        if (tags.is_valid()) {
            for (int j = 0; j < tags.count(); ++j) {
                if (!qstricmp(tags.get(j), "Audio"))
                    return true;
            }
        }
    }
    return false;
}

int Controller::realTime() const
{
    int realtime = 1;
    if (!Settings.playerRealtime()) {
        if (Settings.playerGPU()) {
            return -1;
        } else {
            int threadCount = QThread::idealThreadCount();
            threadCount = threadCount > 2? qMin(threadCount - 1, 4) : 1;
            realtime = -threadCount;
        }
    }
    return realtime;
}

void Controller::setImageDurationFromDefault(Service* service) const
{
    if (service && service->is_valid()) {
        if (isImageProducer(service)) {
            service->set("ttl", 1);
            service->set("length", qRound(m_profile->fps() * kMaxImageDurationSecs));
            service->set("out", qRound(m_profile->fps() * Settings.imageDuration()) - 1);
        }
    }
}

QUuid Controller::uuid(Mlt::Properties &properties) const
{
    return QUuid(properties.get(kUuidProperty));
}

void Controller::setUuid(Mlt::Properties &properties, QUuid uid) const
{
    properties.set(kUuidProperty,
            (uid.toByteArray() + '\n').data());
}

QUuid Controller::ensureHasUuid(Mlt::Properties& properties) const
{
    if (properties.get(kUuidProperty)) {
        return uuid(properties);
    } else {
        QUuid newUid = QUuid::createUuid();
        setUuid(properties, newUid);
        return newUid;
    }
}

void Controller::copyFilters(Producer& fromProducer, Producer& toProducer)
{
    int count = fromProducer.filter_count();
    for (int i = 0; i < count; i++) {
        QScopedPointer<Mlt::Filter> fromFilter(fromProducer.filter(i));
        if (fromFilter && fromFilter->is_valid() && !fromFilter->get_int("_loader") && fromFilter->get("mlt_service")) {
            Mlt::Filter toFilter(MLT.profile(), fromFilter->get("mlt_service"));
            if (toFilter.is_valid()) {
                toFilter.inherit(*fromFilter);
                toProducer.attach(toFilter);
            }
        }
    }
}

void Controller::copyFilters(Mlt::Producer* producer)
{
    if (producer && producer->is_valid()) {
        m_filtersClipboard.reset(new Mlt::Producer(profile(), "color", "black"));
        copyFilters(*producer, *m_filtersClipboard);
    } else if (m_producer && m_producer->is_valid()) {
        m_filtersClipboard.reset(new Mlt::Producer(profile(), "color", "black"));
        copyFilters(*m_producer, *m_filtersClipboard);
    }
}

void Controller::pasteFilters(Mlt::Producer* producer)
{
    Mlt::Producer* targetProducer = (producer && producer->is_valid())? producer
                      :(m_producer && m_producer->is_valid())? m_producer
                      : 0;
    if (targetProducer) {
        copyFilters(*m_filtersClipboard, *targetProducer);

        // Adjust all filters that have an explicit duration.
        int n = targetProducer->filter_count();
        for (int j = 0; j < n; j++) {
            Mlt::Filter* filter = targetProducer->filter(j);
            if (filter && filter->is_valid() && filter->get_length() > 0) {
                if (QString(filter->get(kShotcutFilterProperty)).startsWith("fadeIn")
                        || QString(filter->get("mlt_service")) == "webvfx") {
                    int in = targetProducer->get_int(kFilterInProperty);
                    filter->set_in_and_out(in, in + filter->get_length() - 1);
                }
                else if (QString(filter->get(kShotcutFilterProperty)).startsWith("fadeOut")) {
                    int out = targetProducer->get_int(kFilterOutProperty);
                    filter->set_in_and_out(out - filter->get_length() + 1, out);
                }
            }
            delete filter;
        }
    }
}

void Controller::setSavedProducer(Mlt::Producer* producer)
{
    m_savedProducer.reset(new Mlt::Producer(producer));
}

void TransportControl::play(double speed)
{
    MLT.play(speed);
}

void TransportControl::pause()
{
    MLT.pause();
}

void TransportControl::stop()
{
    MLT.stop();
}

void TransportControl::seek(int position)
{
    MLT.seek(position);
}

void TransportControl::rewind()
{
    MLT.rewind();
}

void TransportControl::fastForward()
{
    MLT.fastForward();
}

void TransportControl::previous(int currentPosition)
{
    MLT.previous(currentPosition);
}

void TransportControl::next(int currentPosition)
{
    MLT.next(currentPosition);
}

void TransportControl::setIn(int in)
{
    MLT.setIn(in);
}

void TransportControl::setOut(int out)
{
    MLT.setOut(out);
}

} // namespace
