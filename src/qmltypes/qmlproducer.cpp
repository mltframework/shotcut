/*
 * Copyright (c) 2016-2020 Meltytech, LLC
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
#include "mltcontroller.h"
#include "util.h"
#include "models/audiolevelstask.h"
#include "mainwindow.h"

static const char *kWidthProperty = "meta.media.width";
static const char *kHeightProperty = "meta.media.height";
static const char *kAspectNumProperty = "meta.media.sample_aspect_num";
static const char *kAspectDenProperty = "meta.media.sample_aspect_den";


QmlProducer::QmlProducer(QObject *parent)
    : QObject(parent)
{
    connect(this, SIGNAL(inChanged(int)), this, SIGNAL(durationChanged()));
    connect(this, SIGNAL(outChanged(int)), this, SIGNAL(durationChanged()));
}

int QmlProducer::in()
{
    if (!m_producer.is_valid()) return 0;
    if (m_producer.get(kFilterInProperty))
        // Shots on the timeline will set the producer to the cut parent.
        // However, we want time-based filters such as fade in/out to use
        // the cut's in/out and not the parent's.
        return m_producer.get_int(kFilterInProperty);
    else
        return m_producer.get_in();
}

int QmlProducer::out()
{
    if (!m_producer.is_valid()) return 0;
    if (m_producer.get(kFilterOutProperty))
        // Shots on the timeline will set the producer to the cut parent.
        // However, we want time-based filters such as fade in/out to use
        // the cut's in/out and not the parent's.
        return m_producer.get_int(kFilterOutProperty);
    else
        return m_producer.get_out();
}

double QmlProducer::aspectRatio()
{
    if (!m_producer.is_valid()) return 1.0;
    if (m_producer.get(kHeightProperty)) {
        double sar = 1.0;
        if (m_producer.get(kAspectDenProperty)) {
            sar = m_producer.get_double(kAspectNumProperty) /
                  m_producer.get_double(kAspectDenProperty);
        }
        return sar * m_producer.get_double(kWidthProperty) / m_producer.get_double(kHeightProperty);
    }
    return MLT.profile().dar();
}

QString QmlProducer::resource()
{
    if (!m_producer.is_valid()) return QString();
    QString result = QString::fromUtf8(m_producer.get("resource"));
    if (result == "<producer>" && m_producer.get("mlt_service"))
        result = QString::fromUtf8(m_producer.get("mlt_service"));
    return result;
}

QString QmlProducer::name()
{
    return Util::producerTitle(m_producer);
}

QVariant QmlProducer::audioLevels()
{
    if (!m_producer.is_valid()) return QVariant();
    if (m_producer.get_data(kAudioLevelsProperty))
        return QVariant::fromValue(*((QVariantList *) m_producer.get_data(kAudioLevelsProperty)));
    else
        return QVariant();
}

int QmlProducer::fadeIn()
{
    if (!m_producer.is_valid()) return 0;
    QScopedPointer<Mlt::Filter> filter(MLT.getFilter("fadeInVolume", &m_producer));
    if (!filter || !filter->is_valid())
        filter.reset(MLT.getFilter("fadeInBrightness", &m_producer));
    if (!filter || !filter->is_valid())
        filter.reset(MLT.getFilter("fadeInMovit", &m_producer));
    return (filter && filter->is_valid()) ? filter->get_length() : 0;
}

int QmlProducer::fadeOut()
{
    if (!m_producer.is_valid()) return 0;
    QScopedPointer<Mlt::Filter> filter(MLT.getFilter("fadeOutVolume", &m_producer));
    if (!filter || !filter->is_valid())
        filter.reset(MLT.getFilter("fadeOutBrightness", &m_producer));
    if (!filter || !filter->is_valid())
        filter.reset(MLT.getFilter("fadeOutMovit", &m_producer));
    return (filter && filter->is_valid()) ? filter->get_length() : 0;
}

double QmlProducer::speed()
{
    double result = 1.0;
    if (!m_producer.is_valid()) return result;
    if (m_producer.is_valid()) {
        if (!qstrcmp("timewarp", m_producer.get("mlt_service")))
            result = m_producer.get_double("warp_speed");
    }
    return result;
}

void QmlProducer::setPosition(int position)
{
    if (!m_producer.is_valid()) return;
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

Q_INVOKABLE bool QmlProducer::outOfBounds()
{
    return m_position < 0 || m_position > duration();
}

void QmlProducer::audioLevelsReady(const QModelIndex &index)
{
    Q_UNUSED(index)
    emit audioLevelsChanged();
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

double QmlProducer::displayAspectRatio()
{
    if (m_producer.is_valid() && m_producer.get(kHeightProperty)) {
        double sar = 1.0;
        if (m_producer.get(kAspectDenProperty)) {
            sar = m_producer.get_double(kAspectNumProperty) /
                  m_producer.get_double(kAspectDenProperty);
        }
        return sar * m_producer.get_double(kWidthProperty) / m_producer.get_double(kHeightProperty);
    }
    return MLT.profile().dar();
}

QString QmlProducer::get(QString name, int position)
{
    if (m_producer.is_valid()) {
        if (position < 0)
            return QString::fromUtf8(m_producer.get(name.toUtf8().constData()));
        else
            return QString::fromUtf8(m_producer.anim_get(name.toUtf8().constData(), position, duration()));
    } else {
        return QString();
    }
}

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

QRectF QmlProducer::getRect(QString name, int position)
{
    if (!m_producer.is_valid()) return QRectF();
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
    if (m_producer.is_valid())
        remakeAudioLevels(MAIN.keyframesDockIsVisible());
    emit producerChanged();
    emit inChanged(0);
    emit outChanged(0);
    emit lengthChanged();
}

