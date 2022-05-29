/*
 * Copyright (c) 2012-2022 Meltytech, LLC
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

#include "abstractproducerwidget.h"
#include <QWidget>

AbstractProducerWidget::AbstractProducerWidget()
{
}

AbstractProducerWidget::~AbstractProducerWidget()
{
}

void AbstractProducerWidget::setProducer(Mlt::Producer *producer)
{
    if (producer) {
        loadPreset(*producer);
        m_producer.reset(new Mlt::Producer(producer));
    } else {
        m_producer.reset();
    }
}

bool AbstractProducerWidget::isDevice(const QWidget *widget)
{
    auto name = widget->objectName();
    return "AlsaWidget" == name || "AvfoundationProducerWidget" == name
           || "DecklinkProducerWidget" == name || "DirectShowVideoWidget" == name
           || "GDIgrabWidget" == name || "PulseAudioWidget" == name
           || "Video4LinuxWidget" == name || "X11grabWidget" == name;
}
