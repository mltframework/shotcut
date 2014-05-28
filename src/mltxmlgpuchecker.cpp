/*
 * Copyright (c) 2014 Meltytech, LLC
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

#include "mltxmlgpuchecker.h"
#include <QIODevice>
#include <QDebug>

MltXmlGpuChecker::MltXmlGpuChecker()
    : m_needsGPU(false)
    , m_hasEffects(false)
{
}

bool MltXmlGpuChecker::check(QIODevice *device)
{
    qDebug() << "begin";
    m_xml.setDevice(device);
    if (m_xml.readNextStartElement()) {
        if (m_xml.name() == "mlt") {
            readMlt();
        } else {
            m_xml.raiseError(QObject::tr("The file is not a MLT XML file."));
        }
    }
    qDebug() << "end";
    return m_xml.error() == QXmlStreamReader::NoError;
}

QString MltXmlGpuChecker::errorString() const
{
    return m_xml.errorString();
}

void MltXmlGpuChecker::readMlt()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "mlt");

    bool isRelevant = false;
    while (!m_xml.atEnd() && !m_needsGPU) {
        m_xml.readNext();
        if (m_xml.isStartElement()) {
            if (m_xml.name() == "filter" || m_xml.name() == "transition") {
                isRelevant = true;
                m_hasEffects = true;
            } else if (isRelevant && m_xml.name() == "property") {
                readProperty();
            }
        } else if (m_xml.isEndElement()) {
            if (m_xml.name() == "filter" || m_xml.name() == "transition")
                isRelevant = false;
        }
    }
}

void MltXmlGpuChecker::readProperty()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "property");

    if (m_xml.attributes().value("name") == "mlt_service") {
        const QString& value = m_xml.readElementText();
        if (value.startsWith("movit.") || value.startsWith("glsl."))
            m_needsGPU = true;
    }
}
