/*
 * Copyright (c) 2014-2025 Meltytech, LLC
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

#include <QFileInfo>
#include <QPair>
#include <QStandardItemModel>
#include <QString>
#include <QTemporaryFile>
#include <QVector>
#include <QVersionNumber>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

class QUIDevice;

class MltXmlChecker
{
public:
    enum { ShotcutHashRole = Qt::UserRole + 1 };

    enum { MissingColumn = 0, ReplacementColumn, ColumnCount };

    MltXmlChecker();
    QXmlStreamReader::Error check(const QString &fileName);
    QString errorString() const;
    bool needsGPU() const { return m_needsGPU; }
    bool needsCPU() const { return m_needsCPU; }
    bool hasEffects() const { return m_hasEffects; }
    bool isCorrected() const { return m_isCorrected; }
    bool isUpdated() const { return m_isUpdated; }
    QTemporaryFile &tempFile() const { return *m_tempFile; }
    QStandardItemModel &unlinkedFilesModel() { return m_unlinkedFilesModel; }
    QString shotcutVersion() const { return m_shotcutVersion; }

private:
    typedef QPair<QString, QString> MltProperty;

    void readMlt();
    void processProperties();
    void checkInAndOutPoints();
    bool checkNumericString(QString &value);
    bool fixWebVfxPath(QString &resource);
    bool readResourceProperty(const QString &name, QString &value);
    void checkGpuEffects(const QString &mlt_service);
    void checkCpuEffects(const QString &mlt_service);
    void checkUnlinkedFile(const QString &mlt_service);
    bool fixUnlinkedFile(QString &value);
    void fixStreamIndex(MltProperty &property);
    bool fixVersion1701WindowsPathBug(QString &value);
    void checkIncludesSelf(QVector<MltProperty> &properties);
    void checkLumaAlphaOver(const QString &mlt_service, QVector<MltProperty> &properties);
    void checkAudioGain(const QString &mlt_service, QVector<MltProperty> &properties);
    void replaceWebVfxCropFilters(QString &mlt_service, QVector<MltProperty> &properties);
    void replaceWebVfxChoppyFilter(QString &mlt_service, QVector<MltProperty> &properties);
    void checkForProxy(const QString &mlt_service, QVector<MltProperty> &properties);
    bool checkMltVersion();

    QXmlStreamReader m_xml;
    QXmlStreamWriter m_newXml;
    bool m_needsGPU;
    bool m_needsCPU;
    bool m_hasEffects;
    bool m_isCorrected;
    bool m_isUpdated;
    QChar m_decimalPoint;
    QScopedPointer<QTemporaryFile> m_tempFile;
    bool m_numericValueChanged;
    QFileInfo m_fileInfo;
    QStandardItemModel m_unlinkedFilesModel;
    QString mlt_class;
    QVector<MltProperty> m_properties;
    struct MltXmlResource
    {
        QFileInfo info;
        QString hash;
        QString newHash;
        QString newDetail;
        QString prefix;
        QString suffix;
        int audio_index, video_index;
        bool isProxy;
        bool notProxyMeta;

        void clear()
        {
            info.setFile(QString());
            hash.clear();
            newHash.clear();
            newDetail.clear();
            prefix.clear();
            suffix.clear();
            audio_index = video_index = -1;
            isProxy = false;
            notProxyMeta = false;
        }
    } m_resource;
    QVersionNumber m_mltVersion;
    QString m_shotcutVersion;
};

#endif // MLTXMLCHECKER_H
