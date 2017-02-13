/*
 * Copyright (c) 2013-2017 Meltytech, LLC
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

#ifndef QMLMETADATA_H
#define QMLMETADATA_H

#include <QObject>
#include <QString>
#include <QDir>
#include <QUrl>

class QmlMetadata : public QObject
{
    Q_OBJECT
    Q_ENUMS(PluginType)
    Q_PROPERTY(PluginType type READ type WRITE setType)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString mlt_service READ mlt_service WRITE set_mlt_service)
    Q_PROPERTY(bool needsGPU READ needsGPU WRITE setNeedsGPU NOTIFY changed)
    Q_PROPERTY(QString qml READ qmlFileName WRITE setQmlFileName)
    Q_PROPERTY(QString vui READ vuiFileName WRITE setVuiFileName)
    Q_PROPERTY(QUrl qmlFilePath READ qmlFilePath )
    Q_PROPERTY(QUrl vuiFilePath READ vuiFilePath )
    Q_PROPERTY(bool isAudio READ isAudio WRITE setIsAudio NOTIFY changed)
    Q_PROPERTY(bool isHidden READ isHidden WRITE setIsHidden NOTIFY changed)
    Q_PROPERTY(bool isFavorite READ isFavorite WRITE setIsFavorite NOTIFY changed)
    Q_PROPERTY(QString gpuAlt READ gpuAlt WRITE setGpuAlt NOTIFY changed)
    Q_PROPERTY(bool allowMultiple READ allowMultiple WRITE setAllowMultiple)
    Q_PROPERTY(bool isClipOnly READ isClipOnly WRITE setIsClipOnly)
    Q_PROPERTY(bool isGpuCompatible READ isGpuCompatible() WRITE setIsGpuCompatible)

public:
    enum PluginType {
        Filter,
        Producer,
        Transition
    };

    explicit QmlMetadata(QObject *parent = 0);
    void loadSettings();

    PluginType type() const { return m_type; }
    void setType(PluginType);
    QString name() const { return m_name; }
    void setName(const QString&);
    QString mlt_service() const { return m_mlt_service; }
    void set_mlt_service(const QString&);
    QString uniqueId() const;
    bool needsGPU() const { return m_needsGPU; }
    void setNeedsGPU(bool);
    QString qmlFileName() const { return m_qmlFileName; }
    void setQmlFileName(const QString&);
    QString vuiFileName() const { return m_vuiFileName; }
    void setVuiFileName(const QString&);
    QDir path() const { return m_path; }
    void setPath(const QDir& path);
    QUrl qmlFilePath() const;
    QUrl vuiFilePath() const;
    bool isAudio() const { return m_isAudio; }
    void setIsAudio(bool isAudio);
    bool isHidden() const { return m_isHidden; }
    void setIsHidden(bool isHidden);
    bool isFavorite() const { return m_isFavorite; }
    void setIsFavorite(bool isFavorite);
    QString gpuAlt() const { return m_gpuAlt; }
    void setGpuAlt(const QString&);
    bool allowMultiple() const { return m_allowMultiple; }
    void setAllowMultiple(bool allowMultiple);
    bool isClipOnly() const { return m_isClipOnly; }
    void setIsClipOnly(bool isClipOnly);
    bool isGpuCompatible() const { return m_isGpuCompatible; }
    void setIsGpuCompatible(bool isCompatible) { m_isGpuCompatible = isCompatible; }

signals:
    void changed();

private:
    PluginType m_type;
    QString m_name;
    QString m_mlt_service;
    bool m_needsGPU;
    QString m_qmlFileName;
    QString m_vuiFileName;
    QDir m_path;
    bool m_isAudio;
    bool m_isHidden;
    bool m_isFavorite;
    QString m_gpuAlt;
    bool m_allowMultiple;
    bool m_isClipOnly;
    bool m_isGpuCompatible;
};

#endif // QMLMETADATA_H
