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
#include <mlt++/Mlt.h>
#include "transportcontrol.h"

// forward declarations
class QWidget;
namespace Mlt {

class QFrame : public QObject
{
    Q_OBJECT
public:
    QFrame(QObject *parent = 0);
    QFrame(const Frame& frame);
    QFrame(const QFrame& qframe);
    ~QFrame();
    Frame* frame() const;
private:
    Frame* m_frame;
};

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
    static Controller& singleton(QWidget* parent = 0);
    virtual ~Controller();

    virtual QWidget* videoWidget() = 0;
    virtual int open(Mlt::Producer*, bool isMulti = false);
    virtual int open(const char* url);
    virtual void close();
    virtual QImage image(Frame *frame, int width, int height);

    void closeConsumer();
    void play(double speed = 1.0);
    void pause();
    void stop();
    bool enableJack(bool enable = true);
    void setVolume(double volume);
    double volume() const;
    void onWindowResize();
    void seek(int position);
    void refreshConsumer();
    QString saveXML(const QString& filename, Service* service = 0);
    int consumerChanged();
    int setProfile(const QString& profile_name);
    QString resource() const;
    bool isSeekable();
    bool isPlaylist() const;
    void rewind();
    void fastForward();
    void previous(int currentPosition);
    void next(int currentPosition);
    void setIn(int);
    void setOut(int);

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

protected:
    Mlt::Repository* m_repo;
    Mlt::Producer* m_producer;
    Mlt::FilteredConsumer* m_consumer;

private:
    Mlt::Profile* m_profile;
    Mlt::Filter* m_volumeFilter;
    Mlt::Filter* m_jackFilter;
    QString m_url;
    double m_volume;
    TransportControl m_transportControl;

    static void on_jack_started(mlt_properties owner, void* object, mlt_position *position);
    void onJackStarted(int position);
    static void on_jack_stopped(mlt_properties owner, void* object, mlt_position *position);
    void onJackStopped(int position);
};

} // namespace

#define MLT Mlt::Controller::singleton()

#endif // MLTCONTROLLER_H
