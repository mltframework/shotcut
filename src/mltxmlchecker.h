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
#include <QVector>
#include <QPair>

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
    bool needsCPU() const { return m_needsCPU; }
    bool hasEffects() const { return m_hasEffects; }
    bool isCorrected() const { return m_isCorrected; }
    QString tempFileName() const { return m_tempFile.fileName(); }
    QStandardItemModel& unlinkedFilesModel() { return m_unlinkedFilesModel; }

private:
    void readMlt();
    void processProperties();
    void checkInAndOutPoints();
    bool checkNumericString(QString& value);
    bool fixWebVfxPath(QString& resource);
    bool readResourceProperty(const QString& name, QString& value);
    void checkGpuEffects(const QString& mlt_service);
    void checkCpuEffects(const QString& mlt_service);
    void checkUnlinkedFile(const QString& mlt_service);
    bool fixUnlinkedFile(QString& value);
    void fixStreamIndex(QString& value);
    bool fixVersion1701WindowsPathBug(QString& value);

    QXmlStreamReader m_xml;
    QXmlStreamWriter m_newXml;
    bool m_needsGPU;
    bool m_needsCPU;
    bool m_hasEffects;
    bool m_isCorrected;
    QChar m_decimalPoint;
    QTemporaryFile m_tempFile;
    bool m_numericValueChanged;
    QString m_basePath;
    QStandardItemModel m_unlinkedFilesModel;
    typedef QPair<QString, QString> MltProperty;
    QString mlt_class;
    QVector<MltProperty> m_properties;
    struct MltXmlResource {
        QFileInfo info;
        QString hash;
        QString newHash;
        QString newDetail;
        QString prefix;

        void clear() {
            info.setFile(QString());
            hash.clear();
            newHash.clear();
            newDetail.clear();
            prefix.clear();
        }
    } m_resource;
};

#endif // MLTXMLCHECKER_H
