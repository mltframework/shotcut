/*
 * Copyright (c) 2014-2016 Meltytech, LLC
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

#ifndef MLTXMLCHECKER_H
#define MLTXMLCHECKER_H

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QTemporaryFile>
#include <QString>

class QUIDevice;

class MltXmlChecker
{
public:
    MltXmlChecker();
    bool check(const QString& fileName);
    QString errorString() const;
    bool needsGPU() const { return m_needsGPU; }
    bool hasEffects() const { return m_hasEffects; }
    bool isCorrected() const { return m_isCorrected; }
    QString tempFileName() const { return m_tempFile.fileName(); }

private:
    void readMlt();
    bool readMltService();
    void checkInAndOutPoints();
    bool checkNumericString(QString& value);
    bool checkNumericProperty();
    bool fixWebVfxPath();

    QXmlStreamReader m_xml;
    QXmlStreamWriter m_newXml;
    bool m_needsGPU;
    bool m_hasEffects;
    bool m_isCorrected;
    QChar m_decimalPoint;
    QTemporaryFile m_tempFile;
    bool m_hasComma;
    bool m_hasPeriod;
    bool m_numericValueChanged;
    QString m_service;
};

#endif // MLTXMLCHECKER_H
