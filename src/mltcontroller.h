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

#ifndef MLTCONTROLLER_H
#define MLTCONTROLLER_H

#include <QImage>
#include <QString>
#include <QUuid>
#include <Mlt.h>
#include "transportcontrol.h"

// forward declarations
class QQuickView;

namespace Mlt {

extern const QString XmlMimeType;

class TransportControl : public TransportControllable
{
    Q_OBJECT
public slots:
    void play(double speed = 1.0);
    void pause();
    void stop();
    void seek(int position);
    void rewind();
    void fastForward();
    void previous(int currentPosition);
    void next(int currentPosition);
    void setIn(int);
    void setOut(int);
};

class Controller
{
protected:
    Controller();
    virtual int reconfigure(bool isMulti) = 0;

public:
    static Controller& singleton(QObject *parent = 0);
    virtual ~Controller();
    static void destroy();

    virtual QObject* videoWidget() = 0;
    virtual QQuickView* videoQuickView() = 0;
    virtual int setProducer(Mlt::Producer*, bool isMulti = false);
    virtual int open(const QString& url);
    bool openXML(const QString& filename);
    virtual void close();
    virtual int displayWidth() const = 0;
    virtual int displayHeight() const = 0;

    void closeConsumer();
    virtual void play(double speed = 1.0);
    virtual void pause();
    void stop();
    bool enableJack(bool enable = true);
    void setVolume(double volume, bool muteOnPause = true);
    double volume() const;
    void onWindowResize();
    virtual void seek(int position);
    void refreshConsumer(bool scrubAudio = false);
    void saveXML(const QString& filename, Service* service = 0);
    QString XML(Service* service = 0);
    int consumerChanged();
    void setProfile(const QString& profile_name);
    QString resource() const;
    bool isSeekable() const;
    bool isClip() const;
    bool isSeekableClip();
    bool isPlaylist() const;
    bool isMultitrack() const;
    bool isImageProducer(Service* service) const;
    void rewind();
    void fastForward();
    void previous(int currentPosition);
    void next(int currentPosition);
    void setIn(int);
    void setOut(int);
    void restart();
    void resetURL();
    QImage image(Frame *frame, int width, int height);
    QImage image(Mlt::Producer& producer, int frameNumber, int width, int height);
    void updateAvformatCaching(int trackCount);
    bool isAudioFilter(const QString& name);
    int realTime() const;
    void setImageDurationFromDefault(Service* service) const;
    QUuid uuid(Mlt::Properties &properties) const;
    void setUuid(Mlt::Properties &properties, QUuid uid) const;
    QUuid ensureHasUuid(Mlt::Properties& properties) const;

    Mlt::Repository* repository() const {
        return m_repo;
    }
    Mlt::Profile& profile() const {
        return *m_profile;
    }
    Mlt::Producer* producer() const {
        return m_producer;
    }
    Mlt::Consumer* consumer() const {
        return m_consumer;
    }
    const QString& URL() const {
        return m_url;
    }
    const TransportControllable* transportControl() const {
        return &m_transportControl;
    }
    Mlt::Producer* savedProducer() const {
        return m_savedProducer;
    }

protected:
    Mlt::Repository* m_repo;
    Mlt::Producer* m_producer;
    Mlt::FilteredConsumer* m_consumer;

private:
    Mlt::Profile* m_profile;
    Mlt::Filter* m_jackFilter;
    QString m_url;
    double m_volume;
    TransportControl m_transportControl;
    Mlt::Producer* m_savedProducer;

    static void on_jack_started(mlt_properties owner, void* object, mlt_position *position);
    void onJackStarted(int position);
    static void on_jack_stopped(mlt_properties owner, void* object, mlt_position *position);
    void onJackStopped(int position);
};

} // namespace

#define MLT Mlt::Controller::singleton()

#endif // MLTCONTROLLER_H
