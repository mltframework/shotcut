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
#include <framework/mlt_types.h>

// forward declarations
class QWidget;
namespace Mlt {
    class Profile;
    class Producer;
    class Consumer;

class Controller
{
protected:
    Controller();

public:
    static Controller* createWidget(QWidget* parent);
    virtual ~Controller();

    /** Open a media file, device, or stream.
     * @param[in] url string of file/device/stream
     * @param[in] profile MLT profile
     * @return 0 if no error. Error code if error.
     */
    virtual int open(const char* url, const char* profile = 0);

    /** Close the media.
     */
    virtual void close();

    /** Start playback.
     */
    virtual void play();

    /** Pause playback.
     */
    virtual void pause();

    /** Set the audio output level.
     * @param volume audio volume in the range [0..1]
     */
    virtual void setVolume(double volume);

    /** Get a QImage for a MLT frame.
     * This is primarily used within a slot connected to the frameReceived signal.
     * @param frame a mlt_frame
     * @return a QImage containing the RGBA image for the frame
     */
    QImage getImage(void* frame);

    Mlt::Profile* profile() const
        { return m_profile; }

    virtual void onWindowResize();

    virtual QWidget* qwidget() = 0;

protected:
    Mlt::Profile* m_profile;
    Mlt::Producer* m_producer;
    Mlt::Consumer* m_consumer;

};

} // namespace

#endif // MLTCONTROLLER_H
