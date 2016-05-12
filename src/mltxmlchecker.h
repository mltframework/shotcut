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
#include <QFileInfo>
#include <QStandardItemModel>

class QUIDevice;

class MltXmlChecker
{
public:

    enum {
        ShotcutHashRole = Qt::UserRole + 1
    };

    enum {
        MissingColumn = 0,
        ReplacementColumn,
        ColumnCount
    };

    MltXmlChecker();
    bool check(const QString& fileName);
    QString errorString() const;
    bool needsGPU() const { return m_needsGPU; }
    bool hasEffects() const { return m_hasEffects; }
    bool isCorrected() const { return m_isCorrected; }
    QString tempFileName() const { return m_tempFile.fileName(); }
    QStandardItemModel& unlinkedFilesModel() { return m_unlinkedFilesModel; }

private:
    void readMlt();
    bool readMltService();
    void checkInAndOutPoints();
    bool checkNumericString(QString& value);
    bool checkNumericProperty();
    bool fixWebVfxPath();
    bool readResourceProperty();
    bool readShotcutHashProperty();
    bool fixUnlinkedFile();
    bool fixShotcutHashProperty();
    bool fixShotcutDetailProperty();
    bool fixShotcutCaptionProperty();
    bool fixAudioIndexProperty();
    bool fixVideoIndexProperty();

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
    QString m_basePath;
    QStandardItemModel m_unlinkedFilesModel;

    struct {
        QString name;
        QFileInfo resource;
        QString hash;
        QString newHash;
        QString newDetail;

        void clear() {
            name.clear();
            resource.setFile(QString());
            hash.clear();
            newHash.clear();
            newDetail.clear();
        }
    } m_service;
};

#endif // MLTXMLCHECKER_H
