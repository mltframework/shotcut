/*
 * Copyright (c) 2014-2015 Meltytech, LLC
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

#include "mltxmlchecker.h"
#include "mltcontroller.h"
#include <QLocale>
#include <QDir>
#include <QDebug>

MltXmlChecker::MltXmlChecker()
    : m_needsGPU(false)
    , m_hasEffects(false)
    , m_decimalPoint(QLocale::system().decimalPoint())
    , m_tempFile(QDir::tempPath().append("/shotcut-XXXXXX.mlt"))
    , m_hasComma(false)
    , m_hasPeriod(false)
    , m_valueChanged(false)
{
}

bool MltXmlChecker::check(const QString& fileName)
{
    qDebug() << "begin";

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) &&
            m_tempFile.open()) {
        m_xml.setDevice(&file);
        m_newXml.setDevice(&m_tempFile);
        if (m_xml.readNextStartElement()) {
            if (m_xml.name() == "mlt") {
                m_newXml.writeStartDocument();
                m_newXml.writeCharacters("\n");
                m_newXml.writeStartElement("mlt");
                foreach (QXmlStreamAttribute a, m_xml.attributes()) {
                    if (a.name().toString().toUpper() != "LC_NUMERIC")
                        m_newXml.writeAttribute(a);
                }
                readMlt();
                m_newXml.writeEndElement();
                m_newXml.writeEndDocument();
                m_isCorrected = m_isCorrected || (m_hasPeriod && m_hasComma && m_valueChanged);
            } else {
                m_xml.raiseError(QObject::tr("The file is not a MLT XML file."));
            }
        }
    }
    if (m_tempFile.isOpen()) {
        m_tempFile.close();
//        m_tempFile.open();
//        qDebug() << m_tempFile.readAll().constData();
//        m_tempFile.close();
    }
    qDebug() << "end";
    return m_xml.error() == QXmlStreamReader::NoError;
}

QString MltXmlChecker::errorString() const
{
    return m_xml.errorString();
}

void MltXmlChecker::readMlt()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "mlt");

    bool checkMltService = false;
    while (!m_xml.atEnd()) {
        switch (m_xml.readNext()) {
        case QXmlStreamReader::Characters:
            m_newXml.writeCharacters(m_xml.text().toString());
            break;
        case QXmlStreamReader::Comment:
            m_newXml.writeComment(m_xml.text().toString());
            break;
        case QXmlStreamReader::DTD:
            m_newXml.writeDTD(m_xml.text().toString());
            break;
        case QXmlStreamReader::EntityReference:
            m_newXml.writeEntityReference(m_xml.name().toString());
            break;
        case QXmlStreamReader::ProcessingInstruction:
            m_newXml.writeProcessingInstruction(m_xml.processingInstructionTarget().toString(), m_xml.processingInstructionData().toString());
            break;
        case QXmlStreamReader::StartDocument:
            m_newXml.writeStartDocument(m_xml.documentVersion().toString(), m_xml.isStandaloneDocument());
            break;
        case QXmlStreamReader::EndDocument:
            m_newXml.writeEndDocument();
            break;
        case QXmlStreamReader::StartElement:
            m_newXml.writeStartElement(m_xml.namespaceUri().toString(), m_xml.name().toString());
            if (m_xml.name() == "filter" || m_xml.name() == "transition") {
                checkMltService = true;
            } else if (m_xml.name() == "property") {
                if (checkMltService && readMltService())
                    continue;
                if (checkNumericProperty())
                    continue;
            }
            checkInAndOutPoints();
            break;
        case QXmlStreamReader::EndElement:
            m_newXml.writeEndElement();
            if (m_xml.name() == "filter" || m_xml.name() == "transition")
                checkMltService = false;
            break;
        default:
            break;
        }
    }
}

bool MltXmlChecker::readMltService()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "property");

    if (m_xml.attributes().value("name") == "mlt_service") {
        m_newXml.writeAttributes(m_xml.attributes());

        const QString& value = m_xml.readElementText();
        if (!MLT.isAudioFilter(value))
            m_hasEffects = true;
        if (value.startsWith("movit.") || value.startsWith("glsl."))
            m_needsGPU = true;
        m_newXml.writeCharacters(value);

        m_newXml.writeEndElement();
        return true;
    }
    return false;
}

void MltXmlChecker::checkInAndOutPoints()
{
    Q_ASSERT(m_xml.isStartElement());

    foreach (QXmlStreamAttribute a, m_xml.attributes()) {
        if (a.name() == "in" || a.name() == "out") {
            QString value = a.value().toString();
            if (checkNumericString(value)) {
                m_newXml.writeAttribute(a.name().toString(), value);
                continue;
            }
        }
        m_newXml.writeAttribute(a);
    }
}

bool MltXmlChecker::checkNumericString(QString& value)
{
    if (!m_hasComma)
        m_hasComma = value.contains(',');
    if (!m_hasPeriod)
        m_hasPeriod = value.contains('.');
    if (!value.contains(m_decimalPoint) &&
            (value.contains('.') || value.contains(','))) {
        value.replace(',', m_decimalPoint);
        value.replace('.', m_decimalPoint);
        m_valueChanged = true;
        return true;
    }
    return false;
}

bool MltXmlChecker::checkNumericProperty()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "property");

    QStringRef name = m_xml.attributes().value("name");
    if (name == "length" || name == "geometry") {
        m_newXml.writeAttributes(m_xml.attributes());

        QString value = m_xml.readElementText();
        checkNumericString(value);
        m_newXml.writeCharacters(value);

        m_newXml.writeEndElement();
        return true;
    }
    return false;
}
