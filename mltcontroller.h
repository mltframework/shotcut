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

#include <QObject>
#include <QImage>
#include <framework/mlt_types.h>

// forward declarations
namespace Mlt {
    class Profile;
    class Producer;
    class Consumer;
}
class GLWidget;

class MltController : public QObject
{
    Q_OBJECT
public:
    explicit MltController(QObject *parent = 0);
    ~MltController();

    /** Initialize the controller.
     */
    void init();

    /** Open a media file, device, or stream.
     * @param[in] url string of file/device/stream
     * @param[in] profile MLT profile
     * @return 0 if no error. Error code if error.
     */
    int open(const char* url, const char* profile = 0);

    /** Close the media.
     */
    void close();

    /** Start playback.
     */
    void play();

    /** Pause playback.
     */
    void pause();

    /** Set the SDL audio output level.
     * @param volume audio volume in the range [0..1]
     */
    void setVolume(double volume);

    /** Get a QImage for a MLT frame.
     * This is primarily used within a slot connected to the frameReceived signal.
     * @param frame a mlt_frame
     * @return a QImage containing the RGBA image for the frame
     */
    QImage getImage(void* frame);

    Mlt::Profile* profile() const
        { return m_profile; }

signals:
    /** This method will be called each time a new frame is available.
     * @param frame pass this opaque frame pointer to getImage()
     * @param position the frame number of this frame representing time
     */
    void frameReceived(void* frame, unsigned position);


public slots:
    void onWindowResize();

private:
    Mlt::Profile* m_profile;
    Mlt::Producer* m_producer;
    Mlt::Consumer* m_consumer;

    static void on_frame_show(mlt_consumer, void* self, mlt_frame frame);

};

#endif // MLTCONTROLLER_H
