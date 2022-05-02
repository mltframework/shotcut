/*
 * Copyright (c) 2011-2022 Meltytech, LLC
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
#include <QSaveFile>
#include <QTextStream>
#include <QThreadPool>
#include <Logger.h>
#include <Mlt.h>
#include <cmath>
#include <clocale>
#include <unistd.h>

#include "glwidget.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "mainwindow.h"
#include "controllers/filtercontroller.h"
#include "qmltypes/qmlmetadata.h"
#include "util.h"
#include "proxymanager.h"

namespace Mlt {

static const int kThumbnailOutSeekFactor = 5;
static Controller *instance = nullptr;
const QString XmlMimeType("application/vnd.mlt+xml");
static const char *kMltXmlPropertyName = "string";

Controller::Controller()
    : m_profile(kDefaultMltProfile)
    , m_previewProfile(kDefaultMltProfile)
    , m_blockRefresh(false)
{
    LOG_DEBUG() << "begin";
    m_repo = Mlt::Factory::init();
    resetLocale();
    initFiltersClipboard();
    updateAvformatCaching(0);
    LOG_DEBUG() << "end";
}

Controller &Controller::singleton(QObject *parent)
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
    LOG_DEBUG() << "begin";
    close();
    closeConsumer();
    LOG_DEBUG() << "end";
}

void Controller::destroy()
{
    delete instance;
}

int Controller::setProducer(Mlt::Producer *producer, bool)
{
    int error = 0;

    if (producer != m_producer.data())
        close();
    if (producer && producer->is_valid()) {
        m_producer.reset(producer);
    } else {
        // Cleanup on error
        error = 1;
        delete producer;
    }
    return error;
}

static bool isFpsDifferent(double a, double b)
{
    return qAbs(a - b) > 0.001;
}

int Controller::open(const QString &url, const QString &urlToSave)
{
    int error = 0;
    Mlt::Producer *newProducer = nullptr;

    close();

    if (Settings.playerGPU() && !profile().is_explicit())
        // Prevent loading normalizing filters, which might be Movit ones that
        // may not have a proper OpenGL context when requesting a sample frame.
        newProducer = new Mlt::Producer(profile(), "abnormal", url.toUtf8().constData());
    else
        newProducer = new Mlt::Producer(profile(), url.toUtf8().constData());
    if (newProducer && newProducer->is_valid()) {
        double fps = profile().fps();
        if (!profile().is_explicit()) {
            profile().from_producer(*newProducer);
            profile().set_width(Util::coerceMultiple(profile().width()));
            profile().set_height(Util::coerceMultiple(profile().height()));
        }
        updatePreviewProfile();
        setPreviewScale(Settings.playerPreviewScale());
        if ( url.endsWith(".mlt") ) {
            // Load the number of audio channels being used when this project was created.
            int channels = newProducer->get_int(kShotcutProjectAudioChannels);
            if (!channels)
                channels = 2;
            m_audioChannels = channels;
            if (newProducer->get_int(kShotcutProjectFolder)) {
                QFileInfo info(url);
                setProjectFolder(info.absolutePath());
                ProxyManager::removePending();
            } else {
                setProjectFolder(QString());
            }
        }
        if (isFpsDifferent(profile().fps(), fps) || (Settings.playerGPU() && !profile().is_explicit())) {
            // Reload with correct FPS or with Movit normalizing filters attached.
            delete newProducer;
            newProducer = new Mlt::Producer(profile(), url.toUtf8().constData());
        }
        if (m_url.isEmpty() && QString(newProducer->get("xml")) == "was here") {
            if (newProducer->get_int("_original_type") != mlt_service_tractor_type ||
                    (newProducer->get_int("_original_type") == mlt_service_tractor_type
                     && newProducer->get(kShotcutXmlProperty)))
                m_url = urlToSave;
        }
        Producer *producer = setupNewProducer(newProducer);
        delete newProducer;
        newProducer = producer;
    } else {
        delete newProducer;
        newProducer = nullptr;
        error = 1;
    }
    m_producer.reset(newProducer);
    return error;
}

bool Controller::openXML(const QString &filename)
{
    bool error = true;
    close();
    Producer *producer = new Mlt::Producer(profile(), "xml", filename.toUtf8().constData());
    if (producer->is_valid()) {
        double fps = profile().fps();
        if (!profile().is_explicit()) {
            profile().from_producer(*producer);
            profile().set_width(Util::coerceMultiple(profile().width()));
            profile().set_height(Util::coerceMultiple(profile().height()));
        }
        updatePreviewProfile();
        setPreviewScale(Settings.playerPreviewScale());
        if (isFpsDifferent(profile().fps(), fps)) {
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
    if (m_profile.is_explicit()) {
        pause();
    } else if (m_consumer && !m_consumer->is_stopped()) {
        m_consumer->stop();
    }
    if (isSeekableClip()) {
        setSavedProducer(m_producer.data());
    }
    m_producer.reset();
}

void Controller::closeConsumer()
{
    if (m_consumer)
        m_consumer->stop();
    m_consumer.reset();
    m_jackFilter.reset();
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
        m_consumer->start();
        refreshConsumer(Settings.playerScrubAudio());
    }
    setVolume(m_volume);
}

bool Controller::isPaused() const
{
    return m_producer && qAbs(m_producer->get_speed()) < 0.1;
}

static void fire_jack_seek_event(mlt_properties jack, int position)
{
    mlt_events_fire(jack, "jack-seek", mlt_event_data_from_int(position));
}

void Controller::pause()
{
    if (m_producer && !isPaused()) {
        m_producer->set_speed(0);
        if (m_consumer && m_consumer->is_valid()) {
            m_producer->seek(m_consumer->position() + 1);
            m_consumer->purge();
            m_consumer->start();
            // The following fixes a bug with frame-dropping. It is possible a video frame rendering
            // was just dropped. Then, Shotcut does not know the latest position. Next, a filter modifies
            // a value, which refreshes the consumer, and the position advances. If that value change
            // creates a keyframe, then a subsequent value change creates an additional keyframe one
            // (or more?) frames after the previous one.
            // https://forum.shotcut.org/t/2-keyframes-created-instead-of-one/11252
            if (m_consumer->get_int("real_time") > 0)
                refreshConsumer();
        }
    }
    if (m_jackFilter) {
        stopJack();
        int position = (m_producer && m_producer->is_valid()) ? m_producer->position() : 0;
        ++m_skipJackEvents;
        fire_jack_seek_event(m_jackFilter->get_properties(), position);
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

void Controller::on_jack_started(mlt_properties, void *object, mlt_event_data data)
{
    if (object)
        (static_cast<Controller *>(object))->onJackStarted(Mlt::EventData(data).to_int());
}

void Controller::onJackStarted(int position)
{
    if (m_producer) {
        m_producer->set_speed(1);
        m_producer->seek(position);
        Controller::refreshConsumer();
    }
}

void Controller::on_jack_stopped(mlt_properties, void *object, mlt_event_data data)
{
    if (object)
        (static_cast<Controller *>(object))->onJackStopped(EventData(data).to_int());
}

void Controller::onJackStopped(int position)
{
    if (m_skipJackEvents) {
        --m_skipJackEvents;
    } else {
        if (m_producer) {
            if (!isPaused()) {
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

void Controller::initFiltersClipboard()
{
    m_filtersClipboard.reset(new Mlt::Producer(profile(), "color", "black"));
    if (m_filtersClipboard->is_valid()) {
        m_filtersClipboard->set(kShotcutFiltersClipboard, 1);
    }
}

bool Controller::enableJack(bool enable)
{
    if (!m_consumer)
        return true;
    if (enable && !m_jackFilter) {
        m_jackFilter.reset(new Mlt::Filter(profile(), "jack", "Shotcut player"));
        if (m_jackFilter->is_valid()) {
            m_jackFilter->set("channels", Settings.playerAudioChannels());
            switch (Settings.playerAudioChannels()) {
            case 8:
                m_jackFilter->set("in_8", "-");
                m_jackFilter->set("out_8", "system:playback_8");
                Q_FALLTHROUGH();
            case 7:
                m_jackFilter->set("in_7", "-");
                m_jackFilter->set("out_7", "system:playback_7");
                Q_FALLTHROUGH();
            case 6:
                m_jackFilter->set("in_6", "-");
                m_jackFilter->set("out_6", "system:playback_6");
                Q_FALLTHROUGH();
            case 5:
                m_jackFilter->set("in_5", "-");
                m_jackFilter->set("out_5", "system:playback_5");
                Q_FALLTHROUGH();
            case 4:
                m_jackFilter->set("in_4", "-");
                m_jackFilter->set("out_4", "system:playback_4");
                Q_FALLTHROUGH();
            case 3:
                m_jackFilter->set("in_3", "-");
                m_jackFilter->set("out_3", "system:playback_3");
                Q_FALLTHROUGH();
            case 2:
                m_jackFilter->set("in_2", "-");
                m_jackFilter->set("out_2", "system:playback_2");
                Q_FALLTHROUGH();
            case 1:
                m_jackFilter->set("in_1", "-");
                m_jackFilter->set("out_1", "system:playback_1");
                Q_FALLTHROUGH();
            default:
                break;
            }
            m_consumer->attach(*m_jackFilter);
            m_consumer->set("audio_off", 1);
            if (isSeekable()) {
                m_jackFilter->listen("jack-started", this, reinterpret_cast<mlt_listener>(on_jack_started));
                m_jackFilter->listen("jack-stopped", this, reinterpret_cast<mlt_listener>(on_jack_stopped));
            }
        } else {
            m_jackFilter.reset();
            return false;
        }
    } else if (!enable && m_jackFilter) {
        m_consumer->detach(*m_jackFilter);
        m_jackFilter.reset();
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
    if (muteOnPause && isPaused()) {
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
    bool scrub = isPaused() ? false : Settings.playerScrubAudio();
    refreshConsumer(scrub);
}

void Controller::seek(int position)
{
    setVolume(m_volume, false);
    if (m_producer) {
        // Always pause before seeking (if not already paused).
        m_producer->set_speed(0);
        m_producer->seek(position);
        if (m_consumer && m_consumer->is_valid()) {
            if (m_consumer->is_stopped()) {
                m_consumer->start();
            } else {
                m_consumer->purge();
                Controller::refreshConsumer(Settings.playerScrubAudio());
            }
        }
    }
    if (m_jackFilter) {
        stopJack();
        ++m_skipJackEvents;
        fire_jack_seek_event(m_jackFilter->get_properties(), position);
    }
}

void Controller::refreshConsumer(bool scrubAudio)
{
    if (!m_blockRefresh && m_consumer) {
        // need to refresh consumer when paused
        m_consumer->set("scrub_audio", scrubAudio);
        m_consumer->set("refresh", 1);
    }
}

bool Controller::saveXML(const QString &filename, Service *service, bool withRelativePaths,
                         QTemporaryFile *tempFile, bool proxy, QString projectNote)
{
    QMutexLocker locker(&m_saveXmlMutex);
    QFileInfo fi(filename);
    Consumer c(profile(), "xml", proxy ? filename.toUtf8().constData() : kMltXmlPropertyName);
    Service s(service ? service->get_service() : m_producer->get_service());
    if (s.is_valid()) {
        // The Shotcut rule for paths in MLT XML is forward slashes as created by QFileDialog and QmlFile.
        QString root = withRelativePaths ? QDir::fromNativeSeparators(fi.absolutePath()) : "";
        s.set(kShotcutProjectAudioChannels, m_audioChannels);
        s.set(kShotcutProjectFolder, m_projectFolder.isEmpty() ? 0 : 1);
        if (!projectNote.isEmpty()) {
            s.set(kShotcutProjectNote, projectNote.toUtf8().constData());
        } else {
            s.clear(kShotcutProjectNote);
        }
        int ignore = s.get_int("ignore_points");
        if (ignore)
            s.set("ignore_points", 0);
        c.set("time_format", "clock");
        c.set("no_meta", 1);
        c.set("store", "shotcut");
        c.set("root", root.toUtf8().constData());
        c.set("no_root", 1);
        c.set("title", QString("Shotcut version ").append(SHOTCUT_VERSION).toUtf8().constData());
        c.connect(s);
        c.start();
        if (ignore)
            s.set("ignore_points", ignore);
        auto xml = QString::fromUtf8(c.get(kMltXmlPropertyName));
        if (!proxy && ProxyManager::filterXML(xml, root)) { // also verifies
            if (tempFile) {
                QTextStream stream(tempFile);
                stream.setCodec("UTF-8");
                stream << xml;
                if (tempFile->error() != QFileDevice::NoError) {
                    LOG_ERROR() << "error while writing MLT XML file" << tempFile->fileName() << ":" <<
                                tempFile->errorString();
                    return false;
                }
            } else {
                QSaveFile file(filename);
                file.setDirectWriteFallback(true);
                if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    LOG_ERROR() << "failed to open MLT XML file for writing" << filename;
                    return false;
                }
                QTextStream stream(&file);
                stream.setCodec("UTF-8");
                stream << xml;
                if (file.error() != QFileDevice::NoError) {
                    LOG_ERROR() << "error while writing MLT XML file" << filename << ":" << file.errorString();
                    return false;
                }
                return file.commit();
            }
        }
    }
    return false;
}

QString Controller::XML(Service *service, bool withProfile, bool withMetadata)
{
    Consumer c(profile(), "xml", kMltXmlPropertyName);
    Service s(service ? service->get_service() : (m_producer
                                                  && m_producer->is_valid()) ? m_producer->get_service() : nullptr);
    if (!s.is_valid())
        return "";
    int ignore = s.get_int("ignore_points");
    if (ignore)
        s.set("ignore_points", 0);
    c.set("time_format", "clock");
    if (!withMetadata)
        c.set("no_meta", 1);
    c.set("no_profile", !withProfile);
    c.set("store", "shotcut");
    c.set("root", "");
    c.connect(s);
    c.start();
    if (ignore)
        s.set("ignore_points", ignore);
    return QString::fromUtf8(c.get(kMltXmlPropertyName));
}

int Controller::consumerChanged()
{
    int error = 0;
    if (m_consumer) {
        bool jackEnabled = !m_jackFilter.isNull();
        m_consumer->stop();
        m_consumer.reset();
        m_jackFilter.reset();
        error = reconfigure(false);
        if (m_consumer) {
            enableJack(jackEnabled);
            setVolume(m_volume);
            m_consumer->start();
        }
    }
    return error;
}

void Controller::setProfile(const QString &profile_name)
{
    LOG_DEBUG() << "setting to profile" << (profile_name.isEmpty() ? "Automatic" : profile_name);
    if (!profile_name.isEmpty()) {
        Mlt::Profile tmp(profile_name.toUtf8().constData());
        m_profile.set_colorspace(tmp.colorspace());
        m_profile.set_frame_rate(tmp.frame_rate_num(), tmp.frame_rate_den());
        m_profile.set_height(Util::coerceMultiple(tmp.height()));
        m_profile.set_progressive(tmp.progressive());
        m_profile.set_sample_aspect(tmp.sample_aspect_num(), tmp.sample_aspect_den());
        m_profile.set_display_aspect(tmp.display_aspect_num(), tmp.display_aspect_den());
        m_profile.set_width(Util::coerceMultiple(tmp.width()));
        m_profile.set_explicit(true);
    } else {
        m_profile.set_explicit(false);
        if (m_producer && m_producer->is_valid()
                && (qstrcmp(m_producer->get("mlt_service"), "color")
                    || qstrcmp(m_producer->get("resource"), "_hide"))) {
            m_profile.from_producer(*m_producer);
            m_profile.set_width(Util::coerceMultiple(m_profile.width()));
        } else {
            // Use a default profile with the dummy hidden color producer.
            Mlt::Profile tmp(kDefaultMltProfile);
            m_profile.set_colorspace(tmp.colorspace());
            m_profile.set_frame_rate(tmp.frame_rate_num(), tmp.frame_rate_den());
            m_profile.set_height(Util::coerceMultiple(tmp.height()));
            m_profile.set_progressive(tmp.progressive());
            m_profile.set_sample_aspect(tmp.sample_aspect_num(), tmp.sample_aspect_den());
            m_profile.set_display_aspect(tmp.display_aspect_num(), tmp.display_aspect_den());
            m_profile.set_width(Util::coerceMultiple(tmp.width()));
        }
    }
    updatePreviewProfile();
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
    resource = QString::fromUtf8(m_producer->get("resource"));
    return resource;
}

bool Controller::isSeekable(Producer *p) const
{
    bool seekable = false;
    Mlt::Producer *producer = p ? p : m_producer.data();
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
                seekable = (service == "color") || service.startsWith("frei0r.") || (service == "tone")
                           || (service == "count") || (service == "noise");
            }
        }
    }
    return seekable;
}

bool Controller::isClip() const
{
    return producer() && producer()->is_valid() && !isPlaylist() && !isMultitrack();
}

bool Controller::isSeekableClip()
{
    return isClip() && isSeekable();
}

bool Controller::isPlaylist() const
{
    return m_producer && m_producer->is_valid() &&
           !m_producer->get_int(kShotcutVirtualClip) &&
           (m_producer->get_int("_original_type") == mlt_service_playlist_type || resource() == "<playlist>");
}

bool Controller::isMultitrack() const
{
    return m_producer && m_producer->is_valid()
           && !m_producer->get_int(kShotcutVirtualClip)
           && (m_producer->get_int("_original_type") == mlt_service_tractor_type || resource() == "<tractor>")
           && (m_producer->get(kShotcutXmlProperty));
}

bool Controller::isImageProducer(Service *service) const
{
    if (service && service->is_valid()) {
        QString serviceName = service->get("mlt_service");
        return (serviceName == "pixbuf" || serviceName == "qimage");
    }
    return false;
}

bool Controller::isFileProducer(Service *service) const
{
    if (service && service->is_valid()) {
        QString serviceName = service->get("mlt_service");
        return (serviceName == "pixbuf" ||
                serviceName == "qimage" ||
                serviceName == "glaxnimate" ||
                serviceName.startsWith("avformat") ||
                serviceName.startsWith("timewarp"));
    }
    return false;
}

void Controller::rewind(bool forceChangeDirection)
{
    if (!m_producer || !m_producer->is_valid())
        return;
    // Starting the consumer when producer at its end fails. So, first seek to
    // frame before last.
    if (m_producer->position() >= m_producer->get_length() - 1)
        m_producer->seek(m_producer->get_length() - 2);
    double speed = m_producer->get_speed();
    if (speed == 0.0) {
        play(-1.0);
    } else {
        stopJack();
        if (forceChangeDirection && speed > 0.0)
            speed = -0.5;
        if (speed < 0.0)
            m_producer->set_speed(speed * 2.0);
        else
            m_producer->set_speed(::floor(speed * 0.5));
        if (m_consumer && m_consumer->is_valid())
            m_consumer->purge();
    }
}

void Controller::fastForward(bool forceChangeDirection)
{
    if (!m_producer || !m_producer->is_valid())
        return;
    double speed = m_producer->get_speed();
    if (speed == 0.0) {
        play(1.0);
    } else {
        stopJack();
        if (forceChangeDirection && speed < 0.0)
            speed = 0.5;
        if (speed > 0.0)
            m_producer->set_speed(speed * 2.0);
        else
            m_producer->set_speed(::ceil(speed * 0.5));
        if (m_consumer && m_consumer->is_valid())
            m_consumer->purge();
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
        int delta = in - m_producer->get_in();
        if (!delta) {
            return;
        }
        adjustClipFilters(*m_producer, m_producer->get_in(), m_producer->get_out(), delta, 0);
        m_producer->set("in", in);
        Controller::refreshConsumer();
    }
}

void Controller::setOut(int out)
{
    if (m_producer && m_producer->is_valid()) {
        int delta = out - m_producer->get_out();
        if (!delta) {
            return;
        }
        adjustClipFilters(*m_producer, m_producer->get_in(), m_producer->get_out(), 0, -delta);
        m_producer->set("out", out);
        Controller::refreshConsumer();
    }
}

void Controller::restart(const QString &xml)
{
    if (!m_consumer || !m_consumer->is_valid() || !m_producer || !m_producer->is_valid())
        return;
    const char *position = m_consumer->frames_to_time(m_consumer->position());
    double speed = m_producer->get_speed();
    QString loadXml = xml;
    if (loadXml.isEmpty())
        loadXml = XML();
    stop();
    if (!setProducer(new Mlt::Producer(profile(), "xml-string", loadXml.toUtf8().constData()))) {
        if (m_producer && m_producer->is_valid())
            m_producer->seek(position);
        play(speed);
    }
}

void Controller::resetURL()
{
    m_url = QString();
}

QImage Controller::image(Mlt::Frame *frame, int width, int height)
{
    QImage result;
    if (frame && frame->is_valid()) {
        if (width > 0 && height > 0) {
            frame->set("rescale.interp", "bilinear");
            frame->set("deinterlace_method", "onefield");
            frame->set("top_field_first", -1);
        }
        mlt_image_format format = mlt_image_rgba;
        const uchar *image = frame->get_image(format, width, height);
        if (image) {
            QImage temp(width, height, QImage::Format_ARGB32);
            memcpy(temp.scanLine(0), image, size_t(width * height * 4));
            result = temp.rgbSwapped();
        }
    } else {
        result = QImage(width, height, QImage::Format_ARGB32);
        result.fill(QColor(Qt::red).rgb());
    }
    return result;
}

QImage Controller::image(Producer &producer, int frameNumber, int width, int height)
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
    int i = QThreadPool::globalInstance()->maxThreadCount() + trackCount * 2;
    mlt_service_cache_set_size(nullptr, "producer_avformat", qMax(4, i));
}

bool Controller::isAudioFilter(const QString &name)
{
    QScopedPointer<Properties> metadata(m_repo->metadata(mlt_service_filter_type,
                                                         name.toLatin1().constData()));
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
        }
#if QT_POINTER_SIZE == 4
        // Limit to 1 rendering thread on 32-bit process to reduce memory usage.
        int threadCount = 1;
#else
        int threadCount = QThread::idealThreadCount();
#endif
        threadCount = threadCount > 2 ? qMin(threadCount - 1, 4) : 1;
        realtime = -threadCount;
    }
    return realtime;
}

void Controller::setImageDurationFromDefault(Service *service) const
{
    if (service && service->is_valid()) {
        if (isImageProducer(service) && !service->get_int("shotcut_sequence")) {
            service->set("ttl", 1);
            service->set("length", service->frames_to_time(qRound(m_profile.fps() * kMaxImageDurationSecs),
                                                           mlt_time_clock));
            service->set("out", qRound(m_profile.fps() * Settings.imageDuration()) - 1);
        }
    }
}

void Controller::setDurationFromDefault(Producer *producer) const
{
    if (producer && producer->is_valid()) {
        int out = qRound(m_profile.fps() * Settings.imageDuration()) - 1;
        if (out >= producer->get_length())
            producer->set("length", producer->frames_to_time(out + 1, mlt_time_clock));
        producer->set("length", producer->frames_to_time(qRound(m_profile.fps() * kMaxImageDurationSecs),
                                                         mlt_time_clock));
        producer->set("out", out);
    }
}

void Controller::lockCreationTime(Producer *producer) const
{
    // Apply the creation_time property on the producer so that it will persist
    // through XML serialization/deserialization.
    if (producer && producer->is_valid() && isFileProducer(producer)) {
        int64_t creation_time = producer->get_creation_time();
        if (creation_time != 0) {
            producer->set_creation_time(creation_time);
        }
    }
}

Producer *Controller::setupNewProducer(Producer *newProducer) const
{
    // Call this function before adding a new producer to Shotcut so that
    // It will be configured correctly. The returned producer must be deleted.
    QString serviceName = newProducer->get("mlt_service");
    if (serviceName == "avformat") {
        // Convert avformat to avformat-novalidate so that XML loads faster.
        newProducer->set("mlt_service", "avformat-novalidate");
    }
    setImageDurationFromDefault(newProducer);
    lockCreationTime(newProducer);
    newProducer->get_length_time(mlt_time_clock);

    if (serviceName.startsWith("avformat")) {
        newProducer->set("mute_on_pause", 0);
        // Encapsulate in a chain to enable timing effects
        if (newProducer->type() != mlt_service_chain_type) {
            Mlt::Chain *chain = new Mlt::Chain(MLT.profile());
            chain->set_source(*newProducer);
            chain->get_length_time(mlt_time_clock);

            // Move all non-loader filters to the chain in case this was a clip-only project.
            int i = 0;
            QScopedPointer<Mlt::Filter> filter(newProducer->filter(i));
            while (filter && filter->is_valid()) {
                if (!filter->get_int("_loader")) {
                    newProducer->detach(*filter);
                    chain->Service::attach(*filter);
                } else {
                    ++i;
                }
                filter.reset(newProducer->filter(i));
            }
            return chain;
        }
    }
    return new Mlt::Producer(newProducer);
}

QUuid Controller::uuid(Mlt::Properties &properties) const
{
    return {properties.get(kUuidProperty)};
}

void Controller::setUuid(Mlt::Properties &properties, QUuid uid) const
{
    properties.set(kUuidProperty,
                   (uid.toByteArray() + '\n').data());
}

QUuid Controller::ensureHasUuid(Mlt::Properties &properties) const
{
    if (properties.get(kUuidProperty)) {
        return uuid(properties);
    }
    QUuid newUid = QUuid::createUuid();
    setUuid(properties, newUid);
    return newUid;
}

void Controller::copyFilters(Producer &fromProducer, Producer &toProducer, bool fromClipboard,
                             bool includeDisabled)
{
    int in = fromProducer.get(kFilterInProperty) ? fromProducer.get_int(kFilterInProperty) :
             fromProducer.get_in();
    int out = fromProducer.get(kFilterOutProperty) ? fromProducer.get_int(
                  kFilterOutProperty) : fromProducer.get_out();
    int count = fromProducer.filter_count();

    for (int i = 0; i < count; i++) {
        QScopedPointer<Mlt::Filter> fromFilter(fromProducer.filter(i));
        if (fromFilter && fromFilter->is_valid() && !fromFilter->get_int("_loader")
                && fromFilter->get("mlt_service")
                && (includeDisabled || !fromFilter->get_int("disable"))) {

            // Determine if filter can be added
            auto metadata = MAIN.filterController()->metadataForService(fromFilter.data());
            if (metadata) {
                if (metadata->isClipOnly() && MLT.isTrackProducer(toProducer)) {
                    continue;
                }
                if (!metadata->allowMultiple()) {
                    std::unique_ptr<Mlt::Filter> existing(getFilter(metadata->uniqueId(), &toProducer));
                    if (existing) {
                        continue;
                    }
                }
            }

            // Add the filter to the target producer
            Mlt::Filter toFilter(MLT.profile(), fromFilter->get("mlt_service"));
            if (toFilter.is_valid()) {
                toFilter.inherit(*fromFilter);
                toProducer.attach(toFilter);

                if (!fromClipboard) {
                    toFilter.set(kFilterInProperty, fromFilter->get_in() - in);
                    if (fromFilter->get_out() != out) {
                        toFilter.set(kFilterOutProperty, fromFilter->get_out() - fromFilter->get_in());
                    }
                }
            }
        }
    }

    if (fromProducer.type() == mlt_service_chain_type && toProducer.type() == mlt_service_chain_type) {
        Mlt::Chain fromChain(fromProducer);
        Mlt::Chain toChain(toProducer);
        count = fromChain.link_count();
        for (int i = 0; i < count; i++) {
            QScopedPointer<Mlt::Link> fromLink(fromChain.link(i));
            if (fromLink && fromLink->is_valid() && fromLink->get("mlt_service")) {
                Mlt::Link toLink(fromLink->get("mlt_service"));
                if (toLink.is_valid()) {
                    toLink.inherit(*fromLink);
                    toChain.attach(toLink);
                }
            }
        }
    }
}

void Controller::copyFilters(Mlt::Producer *producer)
{
    if (producer && producer->is_valid()) {
        initFiltersClipboard();
        copyFilters(*producer, *m_filtersClipboard, false, false);
    } else if (m_producer && m_producer->is_valid()) {
        initFiltersClipboard();
        copyFilters(*m_producer, *m_filtersClipboard, false, false);
    }
}

void Controller::pasteFilters(Mlt::Producer *producer, Producer *fromProducer)
{
    Mlt::Producer *targetProducer = (producer && producer->is_valid()) ? producer
                                    : (m_producer && m_producer->is_valid()) ? m_producer.data()
                                    : nullptr;
    if (targetProducer) {
        int j = targetProducer->filter_count();
        if (fromProducer && fromProducer->is_valid()) {
            copyFilters(*fromProducer, *targetProducer, true);
        } else if (hasFiltersOnClipboard()) {
            copyFilters(*m_filtersClipboard, *targetProducer, true);
        }
        adjustFilters(*targetProducer, j);
    }
}

void Controller::adjustFilters(Producer &producer, int index)
{
    bool changed = false;
    int in = producer.get(kFilterInProperty) ? producer.get_int(kFilterInProperty) : producer.get_in();
    int out = producer.get(kFilterOutProperty) ? producer.get_int(kFilterOutProperty) :
              producer.get_out();
    int n = producer.filter_count();

    for (; index < n; index++) {
        QScopedPointer<Mlt::Filter> filter(producer.filter(index));

        if (filter && filter->is_valid()) {
            QString filterName = filter->get(kShotcutFilterProperty);
            if (filterName.startsWith("fadeIn") && !filter->get(kShotcutAnimInProperty)) {
                // Convert legacy fadeIn filters.
                filter->set(kShotcutAnimInProperty, filter->get_length());
            } else if (filterName.startsWith("fadeOut") && !filter->get(kShotcutAnimOutProperty)) {
                // Convert legacy fadeIn filters.
                filter->set(kShotcutAnimOutProperty, filter->get_length());
            }
            if (!filter->get_int("_loader")) {
                int filterIn = in;
                int filterOut = out;
                if (filter->get(kFilterInProperty))
                    filterIn += filter->get_int(kFilterInProperty);
                if (filter->get(kFilterOutProperty))
                    filterOut = qMin(filterIn + filter->get_int(kFilterOutProperty), out);
                filter->set_in_and_out(filterIn, filterOut);
                changed = true;

                if (filterName == "fadeOutBrightness") {
                    const char *key = filter->get_int("alpha") != 1 ? "alpha" : "level";
                    filter->clear(key);
                    filter->anim_set(key, 1, filter->get_length() - filter->get_int(kShotcutAnimOutProperty));
                    filter->anim_set(key, 0, filter->get_length() - 1);
                } else if (filterName == "fadeOutMovit") {
                    filter->clear("opacity");
                    filter->anim_set("opacity", 1, filter->get_length() - filter->get_int(kShotcutAnimOutProperty), 0,
                                     mlt_keyframe_smooth);
                    filter->anim_set("opacity", 0, filter->get_length() - 1);
                } else if (filterName == "fadeOutVolume") {
                    filter->clear("level");
                    filter->anim_set("level", 0, filter->get_length() - filter->get_int(kShotcutAnimOutProperty));
                    filter->anim_set("level", -60, filter->get_length() - 1);
                } else if (filter->get_int(kShotcutAnimOutProperty) > 0) {
                    // Update simple keyframes.
                    QmlMetadata *meta = MAIN.filterController()->metadataForService(filter.data());
                    if (meta && meta->keyframes()) {
                        foreach (QString name, meta->keyframes()->simpleProperties()) {
                            if (!filter->get_animation(name.toUtf8().constData()))
                                // Cause a string property to be interpreted as animated value.
                                filter->anim_get_double(name.toUtf8().constData(), 0, filter->get_length());
                            Mlt::Animation animation = filter->get_animation(name.toUtf8().constData());
                            if (animation.is_valid()) {
                                int n = animation.key_count();
                                if (n > 1) {
                                    animation.set_length(filter->get_length());
                                    animation.key_set_frame(n - 2, filter->get_length() - filter->get_int(kShotcutAnimOutProperty));
                                    animation.key_set_frame(n - 1, filter->get_length() - 1);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (changed)
        MLT.refreshConsumer();
}

static void shiftKeyframes(Mlt::Service *service, QmlMetadata *meta, int delta)
{
    if (!meta || !meta->keyframes() || meta->keyframes()->parameterCount() < 1) {
        return;
    }
    QmlKeyframesMetadata *keyMeta = meta->keyframes();
    for (int parameterIndex = 0; parameterIndex < keyMeta->parameterCount(); parameterIndex++) {
        QmlKeyframesParameter *paramMeta = keyMeta->parameter(parameterIndex);
        QString name = paramMeta->property();
        Mlt::Animation animation = service->get_animation(qUtf8Printable(name));
        if (animation.is_valid()) {
            // Keyframes are not allowed to have negative positions because
            // there is no way for the user to interact with them.
            // Handle keyframes that will become negative by deleting them.
            if (animation.key_get_frame(0) < delta) {
                // Create a new keyframe at position that will become 0 based on interpolated value
                int previous = 0;
                animation.previous_key(delta, previous);
                mlt_keyframe_type existingType = animation.keyframe_type(previous);
                if (paramMeta->isRectangle()) {
                    auto value = service->anim_get_rect(qUtf8Printable(name), delta);
                    service->anim_set(qUtf8Printable(name), value, delta, 0, existingType);
                } else {
                    double value = service->anim_get_double(qUtf8Printable(name), delta);
                    service->anim_set(qUtf8Printable(name), value, delta, 0, existingType);
                    foreach (QString gangName, paramMeta->gangedProperties()) {
                        Mlt::Animation gangAnim = service->get_animation(qUtf8Printable(gangName));
                        double gangValue = service->anim_get_double(qUtf8Printable(gangName), delta);
                        service->anim_set(qUtf8Printable(gangName), gangValue, delta, existingType);
                    }
                }
                // Remove all keyframes that will be negative position
                int count = animation.key_count();
                for (int keyframeIndex = 0; keyframeIndex < count;) {
                    int frame = animation.key_get_frame(keyframeIndex);
                    if (frame < delta) {
                        animation.remove(frame);
                        animation.interpolate();
                        --count;
                    } else {
                        break;
                    }
                }
                foreach (QString gangName, paramMeta->gangedProperties()) {
                    Mlt::Animation gangAnim = service->get_animation(qUtf8Printable(gangName));
                    int gangKeyCount = gangAnim.key_count();
                    for (int keyframeIndex = 0; keyframeIndex < gangKeyCount;) {
                        int frame = gangAnim.key_get_frame(keyframeIndex);
                        if (frame < delta) {
                            gangAnim.remove(frame);
                            gangAnim.interpolate();
                            gangKeyCount -= 1;
                        } else {
                            break;
                        }
                    }
                }
            }
            // Shift all the keyframes proportional to the delta
            animation.shift_frames(-delta);
            foreach (QString gangName, paramMeta->gangedProperties()) {
                Mlt::Animation gangAnim = service->get_animation(qUtf8Printable(gangName));
                gangAnim.shift_frames(-delta);
            }
        }
    }
}

void Controller::adjustFilter(Mlt::Filter *filter, int in, int out, int inDelta, int outDelta)
{
    if (!filter || !filter->is_valid()) {
        return;
    }

    QString filterName = filter->get(kShotcutFilterProperty);
    QmlMetadata *meta = MAIN.filterController()->metadataForService(filter);

    if ((inDelta || outDelta) && meta && meta->mlt_service().startsWith("vidstab")) {
        filter->clear("results");
    }

    if (inDelta) {
        if (in + inDelta < 0) {
            inDelta = -in;
        }
        if (filter->get_int(kShotcutAnimInProperty) == filter->get_int(kShotcutAnimOutProperty)) {
            // Shift all keyframes proportional to the in delta if they are not simple keyframes
            shiftKeyframes(filter,  meta, inDelta);
        }
        if (filterName.startsWith("fadeIn")) {
            if (!filter->get(kShotcutAnimInProperty)) {
                // Convert legacy fadeIn filters.
                filter->set(kShotcutAnimInProperty, filter->get_length());
            }
            filter->set_in_and_out(in + inDelta, filter->get_out());
            emit MAIN.serviceInChanged(inDelta, filter);
        } else if (!filter->get_int("_loader") && filter->get_in() <= in) {
            filter->set_in_and_out(in + inDelta, filter->get_out());
            emit MAIN.serviceInChanged(inDelta, filter);
        }
    }

    if (outDelta) {
        if (filterName.startsWith("fadeOut")) {
            if (!filter->get(kShotcutAnimOutProperty)) {
                // Convert legacy fadeOut filters.
                filter->set(kShotcutAnimOutProperty, filter->get_length());
            }
            filter->set_in_and_out(filter->get_in(), out - outDelta);
            if (filterName == "fadeOutBrightness") {
                const char *key = filter->get_int("alpha") != 1 ? "alpha" : "level";
                filter->clear(key);
                filter->anim_set(key, 1, filter->get_length() - filter->get_int(kShotcutAnimOutProperty));
                filter->anim_set(key, 0, filter->get_length() - 1);
            } else if (filterName == "fadeOutMovit") {
                filter->clear("opacity");
                filter->anim_set("opacity", 1, filter->get_length() - filter->get_int(kShotcutAnimOutProperty), 0,
                                 mlt_keyframe_smooth);
                filter->anim_set("opacity", 0, filter->get_length() - 1);
            } else if (filterName == "fadeOutVolume") {
                filter->clear("level");
                filter->anim_set("level", 0, filter->get_length() - filter->get_int(kShotcutAnimOutProperty));
                filter->anim_set("level", -60, filter->get_length() - 1);
            }
            emit MAIN.serviceOutChanged(outDelta, filter);
        } else if (!filter->get_int("_loader")  && filter->get_out() >= out) {
            filter->set_in_and_out(filter->get_in(), out - outDelta);
            emit MAIN.serviceOutChanged(outDelta, filter);

            // Update simple keyframes
            if (filter->get_int(kShotcutAnimOutProperty) && meta && meta->keyframes()) {
                foreach (QString name, meta->keyframes()->simpleProperties()) {
                    if (!filter->get_animation(name.toUtf8().constData()))
                        // Cause a string property to be interpreted as animated value.
                        filter->anim_get_double(name.toUtf8().constData(), 0, filter->get_length());
                    Mlt::Animation animation = filter->get_animation(name.toUtf8().constData());
                    if (animation.is_valid()) {
                        int n = animation.key_count();
                        if (n > 1) {
                            animation.set_length(filter->get_length());
                            animation.key_set_frame(n - 2, filter->get_length() - filter->get_int(kShotcutAnimOutProperty));
                            animation.key_set_frame(n - 1, filter->get_length() - 1);
                        }
                    }
                }
            }
        }
    }
}

void Controller::adjustClipFilters(Mlt::Producer &producer, int in, int out, int inDelta,
                                   int outDelta)
{
    for (int j = 0; j < producer.filter_count(); j++) {
        QScopedPointer<Mlt::Filter> filter(producer.filter(j));
        adjustFilter(filter.data(), in, out, inDelta, outDelta);
    }

    // Adjust link in/out
    if (producer.type() == mlt_service_chain_type) {
        Mlt::Chain chain(producer);
        int link_count = chain.link_count();
        for (int j = 0; j < link_count; j++) {
            QScopedPointer<Mlt::Link> link(chain.link(j));
            QmlMetadata *meta = MAIN.filterController()->metadataForService(link.data());
            if (link && link->is_valid()) {
                if (inDelta) {
                    shiftKeyframes(link.data(),  meta, inDelta);
                }
                if (link->get_out() >= out) {
                    link->set_in_and_out(link->get_in(), out - outDelta);
                    emit MAIN.serviceOutChanged(outDelta, link.data());
                }
                if (link->get_in() <= in) {
                    link->set_in_and_out(in + inDelta, link->get_out());
                    emit MAIN.serviceInChanged(inDelta, link.data());
                }
            }
        }
    }
}

void Controller::setSavedProducer(Mlt::Producer *producer)
{
    m_savedProducer.reset(new Mlt::Producer(producer));
}

Filter *Controller::getFilter(const QString &name, Service *service)
{
    for (int i = 0; i < service->filter_count(); i++) {
        Mlt::Filter *filter = service->filter(i);
        if (filter) {
            auto filterName = QString::fromUtf8(filter->get(kShotcutFilterProperty));
            if (filterName.isEmpty()) {
                filterName = QString::fromUtf8(filter->get("mlt_service"));
            }
            if (name == filterName)
                return filter;
            delete filter;
        }
    }
    return nullptr;
}

void Controller::setProjectFolder(const QString &folderName)
{
    m_projectFolder = folderName;
    if (!m_projectFolder.isEmpty())
        Settings.setSavePath(m_projectFolder);
    LOG_DEBUG() << "project folder" << m_projectFolder;
}

QChar Controller::decimalPoint()
{
    QChar result('.');
    Mlt::Producer producer(profile(), "color", "black");
    if (producer.is_valid()) {
        const char *timeString = producer.get_length_time(mlt_time_clock);
        if (qstrlen(timeString) >= 8) // HH:MM:SS.ms
            result = timeString[8];
    }
    return result;
}

void Controller::resetLocale()
{
    ::qputenv(MLT_LC_NAME, "C");
    ::setlocale(MLT_LC_CATEGORY, "C");
    LOG_INFO() << "decimal point .";
}

int Controller::filterIn(Playlist &playlist, int clipIndex)
{
    int result = -1;
    QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
    if (info) {
        QScopedPointer<Mlt::ClipInfo> info2(playlist.clip_info(clipIndex - 1));
        if (info2 && info2->producer && info2->producer->is_valid()
                && info2->producer->get(kShotcutTransitionProperty)) {
            // Factor in a transition left of the clip.
            result = info->frame_in - info2->frame_count;
        } else {
            result = info->frame_in;
        }
    }
    return result;
}

int Controller::filterOut(Playlist &playlist, int clipIndex)
{
    int result = -1;
    QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
    if (info) {
        QScopedPointer<Mlt::ClipInfo> info2(playlist.clip_info(clipIndex + 1));
        if (info2 && info2->producer && info2->producer->is_valid()
                && info2->producer->get(kShotcutTransitionProperty)) {
            // Factor in a transition right of the clip.
            result = info->frame_out + info2->frame_count;
        } else {
            result = info->frame_out;
        }
    }
    return result;
}

void Controller::setPreviewScale(int scale)
{
    auto width = m_profile.width();
    auto height = m_profile.height();
    if (scale > 0) {
        height = MIN(scale, m_profile.height());
        width = (height == m_profile.height()) ? m_profile.width() :
                Util::coerceMultiple(height * m_profile.display_aspect_num() / m_profile.display_aspect_den()
                                     * m_profile.sample_aspect_den()  / m_profile.sample_aspect_num());
    }
    LOG_DEBUG() << width << "x" << height;
    m_previewProfile.set_width(width);
    m_previewProfile.set_height(height);
    if (m_consumer) {
        m_consumer->set("width", width);
        m_consumer->set("height", height);
    }
}

void Controller::updatePreviewProfile()
{
    m_previewProfile.set_colorspace(m_profile.colorspace());
    m_previewProfile.set_frame_rate(m_profile.frame_rate_num(), m_profile.frame_rate_den());
    m_previewProfile.set_width(Util::coerceMultiple(m_profile.width()));
    m_previewProfile.set_height(Util::coerceMultiple(m_profile.height()));
    m_previewProfile.set_progressive(m_profile.progressive());
    m_previewProfile.set_sample_aspect(m_profile.sample_aspect_num(), m_profile.sample_aspect_den());
    m_previewProfile.set_display_aspect(m_profile.display_aspect_num(), m_profile.display_aspect_den());
    m_previewProfile.set_explicit(true);
}

void Controller::purgeMemoryPool()
{
    ::mlt_pool_purge();
}

bool Controller::fullRange(Producer &producer)
{
    bool full = !qstrcmp(producer.get("meta.media.color_range"), "full");
    for (int i = 0; !full && i < producer.get_int("meta.media.nb_streams"); i++) {
        QString key = QString("meta.media.%1.stream.type").arg(i);
        QString streamType(producer.get(key.toLatin1().constData()));
        if (streamType == "video") {
            if (i == producer.get_int("video_index")) {
                key = QString("meta.media.%1.codec.pix_fmt").arg(i);
                QString pix_fmt = QString::fromLatin1(producer.get(key.toLatin1().constData()));
                if (pix_fmt.startsWith("yuvj")) {
                    full = true;
                } else if (pix_fmt.contains("gbr") || pix_fmt.contains("rgb")) {
                    full = true;
                }
            }
        }
    }
    return full;
}

bool Controller::isTrackProducer(Producer &producer)
{
    mlt_service_type service_type = producer.type();
    return service_type == mlt_service_playlist_type ||
           (service_type == mlt_service_tractor_type && producer.get_int(kShotcutXmlProperty));
}

bool Controller::blockRefresh(bool block)
{
    m_blockRefresh = block;
    return m_blockRefresh;
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

void TransportControl::rewind(bool forceChangeDirection)
{
    MLT.rewind(forceChangeDirection);
}

void TransportControl::fastForward(bool forceChangeDirection)
{
    MLT.fastForward(forceChangeDirection);
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
