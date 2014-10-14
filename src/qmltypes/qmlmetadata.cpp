/*
 * Copyright (c) 2013 Meltytech, LLC
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

#include "qmlmetadata.h"
#include "settings.h"

QmlMetadata::QmlMetadata(QObject *parent)
    : QObject(parent)
    , m_type(Filter)
    , m_needsGPU(false)
    , m_qmlFileName("ui.qml")
    , m_vuiFileName("vui.qml")
    , m_isAudio(false)
    , m_isHidden(false)
    , m_isFavorite(false)
{
}

void QmlMetadata::loadSettings()
{
    //Override the default favorite setting if it has been set by the user.
    QString favorite = Settings.filterFavorite(uniqueId());
    if (favorite == "yes") {
        m_isFavorite = true;
    } else if (favorite == "no") {
        m_isFavorite = false;
    }
}

void QmlMetadata::setType(QmlMetadata::PluginType type)
{
    m_type = type;
}

void QmlMetadata::setName(const QString &name)
{
    m_name = name;
}

void QmlMetadata::set_mlt_service(const QString &service)
{
    m_mlt_service = service;
}

QString QmlMetadata::uniqueId() const
{
    if (!objectName().isEmpty()) {
        return objectName();
    } else {
        return mlt_service();
    }
}

void QmlMetadata::setNeedsGPU(bool needs)
{
    m_needsGPU = needs;
}

void QmlMetadata::setQmlFileName(const QString &fileName)
{
    m_qmlFileName = fileName;
}

void QmlMetadata::setVuiFileName(const QString &fileName)
{
    m_vuiFileName = fileName;
}

void QmlMetadata::setPath(const QDir &path)
{
    m_path = path;
}

QString QmlMetadata::qmlFilePath() const
{
    return m_path.absoluteFilePath(m_qmlFileName);
}

QString QmlMetadata::vuiFilePath() const
{
    return m_path.absoluteFilePath(m_vuiFileName);
}

void QmlMetadata::setIsAudio(bool isAudio)
{
    m_isAudio = isAudio;
}

void QmlMetadata::setIsHidden(bool isHidden)
{
    m_isHidden = isHidden;
}

void QmlMetadata::setIsFavorite(bool isFavorite)
{
    m_isFavorite = isFavorite;

    if (!uniqueId().isEmpty()) {
        if (isFavorite) {
            Settings.setFilterFavorite(uniqueId(), "yes");
        } else {
            Settings.setFilterFavorite(uniqueId(), "no");
        }
    }
}
