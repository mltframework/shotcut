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
#include <Mlt.h>

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
    QImage image();
private:
    Frame* m_frame;
};

class Controller
{
protected:
    Controller();
    virtual int reconfigure() = 0;

public:
    static Controller& singleton(QWidget* parent = 0);
    virtual ~Controller();

    virtual int open(Mlt::Producer*);
    virtual int open(const char* url);

    /** Close the media.
     */
    virtual void close();

    /** Start playback.
     */
    void play(double speed = 1.0);

    /** Pause playback.
     */
    void pause();

    void stop();

    /** Set the audio output level.
     * @param volume audio volume in the range [0..1]
     */
    void setVolume(double volume);

    void onWindowResize();

    virtual QWidget* videoWidget() = 0;

    void seek(int position);
    void refreshConsumer();
    void saveXML(QString& filename);
    int consumerChanged();
    int setProfile(const QString& profile_name);

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

protected:
    Mlt::Repository* m_repo;
    Mlt::Producer* m_producer;
    Mlt::FilteredConsumer* m_consumer;

private:
    Mlt::Profile* m_profile;
    Mlt::Filter* m_volumeFilter;
};

} // namespace

#define MLT Mlt::Controller::singleton()

#endif // MLTCONTROLLER_H
