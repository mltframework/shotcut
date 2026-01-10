/*
 * Copyright (c) 2015 Meltytech, LLC
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

#ifndef SHAREDFRAME_H
#define SHAREDFRAME_H

#include <MltFrame.h>
#include <QExplicitlySharedDataPointer>
#include <QObject>

#include <stdint.h>

class FrameData;

/*!
  \class SharedFrame
  \brief The SharedFrame provides thread safe access to Mlt::Frame data.

  \threadsafe

  SharedFrame is a wrapper around Mlt::Frame that provides read-only access to
  the frame data. SharedFrame is a reference counted object having only const
  functions. Therefore, it is suitable for concurrent access.

  A SharedFrame can be safely copied. However, all copies will be accessing the
  same wrapped Mlt::Frame. Therefore, SharedFrame can not provide non-const
  access to any of the frame data. If it is necessary for an object to modify
  the frame data (e.g. to resize the image), then the object must call clone()
  to receive its own non-const copy of the frame.

  TODO: Consider providing a similar class in Mlt++.
*/

class SharedFrame
{
public:
    SharedFrame();
    SharedFrame(Mlt::Frame &frame);
    SharedFrame(const SharedFrame &other);
    ~SharedFrame();
    SharedFrame &operator=(const SharedFrame &other);

    bool is_valid() const;
    Mlt::Frame clone(bool audio = false, bool image = false, bool alpha = false) const;
    int get_int(const char *name) const;
    int64_t get_int64(const char *name) const;
    double get_double(const char *name) const;
    int get_position() const;
    mlt_image_format get_image_format() const;
    int get_image_width() const;
    int get_image_height() const;
    const uint8_t *get_image(mlt_image_format format) const;
    mlt_audio_format get_audio_format() const;
    int get_audio_channels() const;
    int get_audio_frequency() const;
    int get_audio_samples() const;
    const int16_t *get_audio() const;
    Mlt::Producer *get_original_producer();

private:
    QExplicitlySharedDataPointer<FrameData> d;
};

#endif // SHAREDFRAME_H
