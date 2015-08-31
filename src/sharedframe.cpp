/*
 * Copyright (c) 2015 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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
#include "sharedframe.h"

class FrameData : public QSharedData
{
public:
    FrameData() : f((mlt_frame)0) {};
    FrameData(Mlt::Frame& frame) : f(frame) {};
    ~FrameData() {};

    Mlt::Frame f;
private:
    Q_DISABLE_COPY(FrameData)
};

SharedFrame::SharedFrame()
  : d(new FrameData)
{
}

SharedFrame::SharedFrame(Mlt::Frame& frame)
  : d(new FrameData(frame))
{
}

SharedFrame::SharedFrame(const SharedFrame& other)
  : d(other.d)
{
}

SharedFrame::~SharedFrame()
{
}

SharedFrame& SharedFrame::operator=(const SharedFrame& other)
{
   d = other.d;
   return *this;
}

bool SharedFrame::is_valid() const
{
    return d->f.is_valid();
}

Mlt::Frame SharedFrame::clone(bool audio, bool image, bool alpha) const
{
    // TODO: Consider moving this implementation into MLT.
    // It could be added to mlt_frame as an alternative to:
    //     mlt_frame mlt_frame_clone( mlt_frame self, int is_deep );
    // It could also be added to Mlt::Frame as a const function.
    void* data = 0;
    void* copy = 0;
    int size = 0;
    Mlt::Frame cloneFrame(mlt_frame_init( NULL ));
    cloneFrame.inherit(d->f);
    cloneFrame.set("_producer", d->f.get_data("_producer", size), 0, NULL, NULL);
    cloneFrame.set("movit.convert", d->f.get_data("movit.convert", size), 0, NULL, NULL);
    cloneFrame.get_frame()->convert_image = d->f.get_frame()->convert_image;
    cloneFrame.get_frame()->convert_audio = d->f.get_frame()->convert_audio;

    data = d->f.get_data("audio", size);
    if (audio && data) {
        if (!size) {
            size = mlt_audio_format_size(get_audio_format(),
                                         get_audio_samples(),
                                         get_audio_channels());
        }
        copy = mlt_pool_alloc(size);
        memcpy(copy, data, size);
        cloneFrame.set("audio", copy, size, mlt_pool_release);
    } else {
        cloneFrame.set("audio", 0);
        cloneFrame.set("audio_format", mlt_audio_none);
        cloneFrame.set("audio_channels", 0);
        cloneFrame.set("audio_frequency", 0);
        cloneFrame.set("audio_samples", 0);
    }

    data = d->f.get_data("image", size );
    if (image && data)
    {
        if (!size) {
            size = mlt_image_format_size(get_image_format(),
                                         get_image_width(),
                                         get_image_height(),
                                         0);
        }
        copy = mlt_pool_alloc(size);
        memcpy(copy, data, size);
        cloneFrame.set("image", copy, size, mlt_pool_release);
    } else {
        cloneFrame.set("image", 0);
        cloneFrame.set("image_format", mlt_image_none);
        cloneFrame.set("width", 0);
        cloneFrame.set("height", 0);
    }

    data = d->f.get_data("alpha", size );
    if (alpha && data)
    {
        if (!size) {
            size = get_image_width() * get_image_height();
        }
        copy = mlt_pool_alloc(size);
        memcpy(copy, data, size);
        cloneFrame.set("alpha", copy, size, mlt_pool_release);
    } else {
        cloneFrame.set("alpha", 0);
    }

    // Release the reference on the initial frame so that the returned frame
    // only has one reference.
    mlt_frame_close(cloneFrame.get_frame());
    return cloneFrame;
}

int SharedFrame::get_int(const char *name) const
{
    return d->f.get_int(name);
}

int64_t SharedFrame::get_int64(const char *name) const
{
    return d->f.get_int64(name);
}

double SharedFrame::get_double(const char *name) const
{
    return d->f.get_double(name);
}

int SharedFrame::get_position() const
{
    return d->f.get_position();
}

mlt_image_format SharedFrame::get_image_format() const
{
    return (mlt_image_format)d->f.get_int( "format" );
}

int SharedFrame::get_image_width() const
{
    return d->f.get_int( "width" );
}

int SharedFrame::get_image_height() const
{
    return d->f.get_int( "height" );
}

const uint8_t* SharedFrame::get_image() const
{
    mlt_image_format format = get_image_format();
    int width = get_image_width();
    int height = get_image_height();
    return (uint8_t*)d->f.get_image(format, width, height, 0);
}

mlt_audio_format SharedFrame::get_audio_format() const
{
    return (mlt_audio_format)d->f.get_int( "audio_format" );
}

int SharedFrame::get_audio_channels() const
{
    return d->f.get_int( "audio_channels" );
}

int SharedFrame::get_audio_frequency() const
{
    return d->f.get_int( "audio_frequency" );
}

int SharedFrame::get_audio_samples() const
{
    return d->f.get_int( "audio_samples" );
}

const int16_t* SharedFrame::get_audio() const
{
    mlt_audio_format format = get_audio_format();
    int frequency = get_audio_frequency();
    int channels = get_audio_channels();
    int samples = get_audio_samples();
    return (int16_t*)d->f.get_audio(format, frequency, channels, samples);
}
