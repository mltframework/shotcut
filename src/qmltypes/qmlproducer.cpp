/*
 * Copyright (c) 2016-2017 Meltytech, LLC
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

#include "qmlproducer.h"
#include "mltcontroller.h"
#include "util.h"

static const char* kWidthProperty = "meta.media.width";
static const char* kHeightProperty = "meta.media.height";
static const char* kAspectNumProperty = "meta.media.sample_aspect_num";
static const char* kAspectDenProperty = "meta.media.sample_aspect_den";


QmlProducer::QmlProducer(Mlt::Producer& producer, QObject *parent)
    : QObject(parent)
    , m_producer(producer)
{

}

int QmlProducer::in()
{
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
    QString result = QString::fromUtf8(m_producer.get("resource"));
    if (result == "<producer>" && m_producer.get("mlt_service"))
        result = QString::fromUtf8(m_producer.get("mlt_service"));
    return result;
}

QString QmlProducer::name()
{
    QString result;
    if (m_producer.is_valid())
        result = m_producer.get(kShotcutCaptionProperty);
    if (result.isNull())
        result = Util::baseName(QString::fromUtf8(m_producer.get("resource")));
    if (result == "<producer>" && m_producer.get("mlt_service"))
        result = QString::fromUtf8(m_producer.get("mlt_service"));
    return result;
}

QVariant QmlProducer::audioLevels()
{
    if (m_producer.get_data(kAudioLevelsProperty))
        return QVariant::fromValue(*((QVariantList*) m_producer.get_data(kAudioLevelsProperty)));
    else
        return QVariant();
}

int QmlProducer::fadeIn()
{
    QScopedPointer<Mlt::Filter> filter(MLT.getFilter("fadeInVolume", &m_producer));
    if (!filter || !filter->is_valid())
        filter.reset(MLT.getFilter("fadeInBrightness", &m_producer));
    if (!filter || !filter->is_valid())
        filter.reset(MLT.getFilter("fadeInMovit", &m_producer));
    return (filter && filter->is_valid())? filter->get_length() : 0;
}

int QmlProducer::fadeOut()
{
    QScopedPointer<Mlt::Filter> filter(MLT.getFilter("fadeOutVolume", &m_producer));
    if (!filter || !filter->is_valid())
        filter.reset(MLT.getFilter("fadeOutBrightness", &m_producer));
    if (!filter || !filter->is_valid())
        filter.reset(MLT.getFilter("fadeOutMovit", &m_producer));
    return (filter && filter->is_valid())? filter->get_length() : 0;
}

double QmlProducer::speed()
{
    double result = 1.0;
    if (m_producer.is_valid()) {
        if (!qstrcmp("timewarp", m_producer.get("mlt_service")))
            result = m_producer.get_double("warp_speed");
    }
    return result;
}

