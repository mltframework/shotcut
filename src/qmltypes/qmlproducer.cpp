/*
 * Copyright (c) 2016-2026 Meltytech, LLC
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

#include "qmlproducer.h"

#include "Logger.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/audiolevelstask.h"
#include "qmltypes/qmlapplication.h"
#include "util.h"
#include "widgets/glaxnimateproducerwidget.h"

static const char *kWidthProperty = "meta.media.width";
static const char *kHeightProperty = "meta.media.height";
static const char *kAspectNumProperty = "meta.media.sample_aspect_num";
static const char *kAspectDenProperty = "meta.media.sample_aspect_den";

/*!
    \qmltype Producer
    \inqmlmodule org.shotcut.qml
    \brief Represents the clip currently loaded in the \b Filters panel, accessed via the \c producer context property.

    \c producer describes the clip that the currently displayed filter is attached to.
    It is available in the Filters panel as the \c producer context property.

    \code
    var startFrame = producer.in
    var endFrame   = producer.out
    var fps        = profile.fps
    \endcode
*/

/*!
    \qmlsignal Producer::producerChanged()
    \brief Emitted when the underlying producer or its properties change.
*/

/*!
    \qmlsignal Producer::positionChanged(int position)
    \brief Emitted when the playhead \a position within the clip changes.
*/

/*!
    \qmlsignal Producer::seeked(int position)
    \brief Emitted when the clip is seeked to time \a position.
*/

/*!
    \qmlsignal Producer::inChanged(int delta)
    \brief Emitted when the clip's in-point changes. \a delta is the frame offset.
*/

/*!
    \qmlsignal Producer::outChanged(int delta)
    \brief Emitted when the clip's out-point changes. \a delta is the frame offset.
*/

/*!
    \qmlsignal Producer::durationChanged()
    \brief Emitted when the clip's duration changes.
*/

/*!
    \qmlsignal Producer::lengthChanged()
    \brief Emitted when the source media length changes.
*/

/*!
    \qmlproperty int Producer::duration
    \brief The duration of the clip in frames (\c out - \c in + 1).
*/

/*!
    \qmlproperty int Producer::length
    \brief The total length of the underlying source media in frames.
*/

/*!
    \qmlproperty string Producer::mlt_service
    \brief The MLT service name of the producer (e.g. \c "avformat", \c "color").
*/

/*!
    \qmlproperty string Producer::hash
    \brief A content-based hash of the producer's media, used for caching.
*/

/*!
    \qmlproperty int Producer::position
    \brief The current playhead position within the clip, in frames relative to \l in.
    Setting this seeks the preview to that position.
*/

QmlProducer::QmlProducer(QObject *parent)
    : QObject(parent)
{
    connect(this, SIGNAL(inChanged(int)), this, SIGNAL(durationChanged()));
    connect(this, SIGNAL(outChanged(int)), this, SIGNAL(durationChanged()));
}

/*!
    \qmlproperty int Producer::in
    \brief The in-point of the clip in frames (absolute timeline position).
*/

int QmlProducer::in()
{
    if (!m_producer.is_valid())
        return 0;
    if (m_producer.get(kFilterInProperty))
        // Shots on the timeline will set the producer to the cut parent.
        // However, we want time-based filters such as fade in/out to use
        // the cut's in/out and not the parent's.
        return m_producer.get_int(kFilterInProperty);
    else
        return m_producer.get_in();
}

/*!
    \qmlproperty int Producer::out
    \brief The out-point of the clip in frames (absolute timeline position).
*/

int QmlProducer::out()
{
    if (!m_producer.is_valid())
        return 0;
    if (m_producer.get(kFilterOutProperty))
        // Shots on the timeline will set the producer to the cut parent.
        // However, we want time-based filters such as fade in/out to use
        // the cut's in/out and not the parent's.
        return m_producer.get_int(kFilterOutProperty);
    else
        return m_producer.get_out();
}

/*!
    \qmlproperty int Producer::aspectRatio
    \brief The pixel aspect ratio of the producer's media.
*/

double QmlProducer::aspectRatio()
{
    if (!m_producer.is_valid())
        return 1.0;
    if (m_producer.get(kHeightProperty)) {
        double sar = 1.0;
        if (m_producer.get(kAspectDenProperty)) {
            sar = m_producer.get_double(kAspectNumProperty)
                  / m_producer.get_double(kAspectDenProperty);
        }
        return sar * m_producer.get_double(kWidthProperty) / m_producer.get_double(kHeightProperty);
    }
    return MLT.profile().dar();
}

/*!
    \qmlproperty string Producer::resource
    \brief The file path or resource string of the underlying media.
*/

QString QmlProducer::resource()
{
    if (!m_producer.is_valid())
        return QString();
    QString result = QString::fromUtf8(m_producer.get("resource"));
    if (result == "<producer>" && m_producer.get("mlt_service"))
        result = QString::fromUtf8(m_producer.get("mlt_service"));
    return result;
}

/*!
    \qmlproperty string Producer::name
    \brief The display name of the clip as shown in the timeline.
*/

QString QmlProducer::name()
{
    return Util::producerTitle(m_producer);
}

const QByteArray *QmlProducer::audioLevels()
{
    if (!m_producer.is_valid())
        return nullptr;
    return static_cast<const QByteArray *>(m_producer.get_data(kAudioLevelsProperty));
}

/*!
    \qmlproperty int Producer::fadeIn
    \brief Duration in frames of the clip's fade-in, or 0 if none.
*/

int QmlProducer::fadeIn()
{
    if (!m_producer.is_valid())
        return 0;
    QScopedPointer<Mlt::Filter> filter(MLT.getFilter("fadeInVolume", &m_producer));
    if (!filter || !filter->is_valid())
        filter.reset(MLT.getFilter("fadeInBrightness", &m_producer));
    if (!filter || !filter->is_valid())
        filter.reset(MLT.getFilter("fadeInMovit", &m_producer));
    return (filter && filter->is_valid()) ? filter->get_length() : 0;
}

/*!
    \qmlproperty int Producer::fadeOut
    \brief Duration in frames of the clip's fade-out, or 0 if none.
*/

int QmlProducer::fadeOut()
{
    if (!m_producer.is_valid())
        return 0;
    QScopedPointer<Mlt::Filter> filter(MLT.getFilter("fadeOutVolume", &m_producer));
    if (!filter || !filter->is_valid())
        filter.reset(MLT.getFilter("fadeOutBrightness", &m_producer));
    if (!filter || !filter->is_valid())
        filter.reset(MLT.getFilter("fadeOutMovit", &m_producer));
    return (filter && filter->is_valid()) ? filter->get_length() : 0;
}

/*!
    \qmlproperty real Producer::speed
    \brief The playback speed multiplier of the clip (1.0 = normal speed).
*/

double QmlProducer::speed()
{
    double result = 1.0;
    if (!m_producer.is_valid())
        return result;
    if (m_producer.is_valid()) {
        if (!qstrcmp("timewarp", m_producer.get("mlt_service")))
            result = m_producer.get_double("warp_speed");
    }
    return result;
}

void QmlProducer::setPosition(int position)
{
    if (!m_producer.is_valid())
        return;
    int length = duration();
    if (position < length) {
        if (MLT.isMultitrack())
            emit seeked(m_producer.get_int(kPlaylistStartProperty) + qMax(0, position));
        else
            emit seeked(in() + qMax(0, position));
    } else if (m_position != length - 1) {
        m_position = length - 1;
        emit positionChanged(m_position);
    }
}

void QmlProducer::seek(int position)
{
    if (m_producer.is_valid() && m_position != position) {
        m_position = position;
        emit positionChanged(qBound(0, position, duration()));
    }
}

/*!
    \qmlmethod bool Producer::outOfBounds()
    \brief Returns \c true if the clip's in/out points fall outside the media's
    available length.
*/

Q_INVOKABLE bool QmlProducer::outOfBounds()
{
    return m_position < 0 || m_position > duration();
}

void QmlProducer::newGlaxnimateFile(const QString &filename)
{
    GlaxnimateIpcServer::instance().newFile(filename, duration());
}

void QmlProducer::launchGlaxnimate(const QString &filename) const
{
    if (!filename.isEmpty()) {
        GlaxnimateIpcServer::instance().launch(m_producer, filename, false);
    }
}

void QmlProducer::remakeAudioLevels()
{
    AudioLevelsTask::start(m_producer, this, QModelIndex(), true);
}

void QmlProducer::remakeAudioLevels(bool isKeyframesVisible)
{
    if (isKeyframesVisible)
        AudioLevelsTask::start(m_producer, this, QModelIndex());
}

/*!
    \qmlproperty real Producer::displayAspectRatio
    \brief The display aspect ratio (DAR) of the producer's media.
*/

double QmlProducer::displayAspectRatio()
{
    if (m_producer.is_valid() && m_producer.get(kHeightProperty)) {
        double sar = 1.0;
        if (m_producer.get(kAspectDenProperty)) {
            sar = m_producer.get_double(kAspectNumProperty)
                  / m_producer.get_double(kAspectDenProperty);
        }
        return sar * m_producer.get_double(kWidthProperty) / m_producer.get_double(kHeightProperty);
    }
    return MLT.profile().dar();
}

/*!
    \qmlmethod string Producer::get(string name, int position = -1)
    \brief Returns the value of MLT property \a name as a string.
    If time \a position is \c -1, returns the current value.
*/

QString QmlProducer::get(QString name, int position)
{
    if (m_producer.is_valid()) {
        if (position < 0)
            return QString::fromUtf8(m_producer.get(name.toUtf8().constData()));
        else
            return QString::fromUtf8(
                m_producer.anim_get(name.toUtf8().constData(), position, duration()));
    } else {
        return QString();
    }
}

/*!
    \qmlmethod real Producer::getDouble(string name, int position = -1)
    \brief Returns the value of MLT property \a name as a floating-point number.
    If time \a position is \c -1, returns the current value.
*/

double QmlProducer::getDouble(QString name, int position)
{
    if (m_producer.is_valid()) {
        if (position < 0)
            return m_producer.get_double(name.toUtf8().constData());
        else
            return m_producer.anim_get_double(name.toUtf8().constData(), position, duration());
    } else {
        return 0.0;
    }
}

/*!
    \qmlmethod rect Producer::getRect(string name, int position = -1)
    \brief Returns the value of MLT property \a name as a \c rect.
    If time \a position is \c -1, returns the current value.
*/

QRectF QmlProducer::getRect(QString name, int position)
{
    if (!m_producer.is_valid())
        return QRectF();
    QString s = QString::fromUtf8(m_producer.get(name.toUtf8().constData()));
    if (!s.isEmpty()) {
        mlt_rect rect;
        if (position < 0) {
            rect = m_producer.get_rect(name.toUtf8().constData());
        } else {
            rect = m_producer.anim_get_rect(name.toUtf8().constData(), position, duration());
        }
        if (s.contains('%')) {
            return QRectF(qRound(rect.x * MLT.profile().width()),
                          qRound(rect.y * MLT.profile().height()),
                          qRound(rect.w * MLT.profile().width()),
                          qRound(rect.h * MLT.profile().height()));
        } else {
            return QRectF(rect.x, rect.y, rect.w, rect.h);
        }
    } else {
        return QRectF(0.0, 0.0, 0.0, 0.0);
    }
}

void QmlProducer::setProducer(Mlt::Producer &producer)
{
    m_producer = producer;
    if (m_producer.is_valid()) {
        remakeAudioLevels(MAIN.keyframesDockIsVisible());
    } else {
        GlaxnimateIpcServer::instance().reset();
    }
    emit producerChanged();
    emit inChanged(0);
    emit outChanged(0);
    emit lengthChanged();
}
