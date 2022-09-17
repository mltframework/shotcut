/*
 * Copyright (c) 2013-2022 Meltytech, LLC
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
#include "util.h"
#include <Logger.h>
#include <QVersionNumber>

QmlMetadata::QmlMetadata(QObject *parent)
    : QObject(parent)
    , m_type(Filter)
    , m_needsGPU(false)
    , m_qmlFileName("")
    , m_vuiFileName("")
    , m_isAudio(false)
    , m_isHidden(false)
    , m_isFavorite(false)
    , m_gpuAlt("")
    , m_allowMultiple(true)
    , m_isClipOnly(false)
    , m_isGpuCompatible(true)
    , m_isDeprecated(false)
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
    emit changed();
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
    emit changed();
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

QUrl QmlMetadata::qmlFilePath() const
{
    QUrl retVal = QUrl();
    if (!m_qmlFileName.isEmpty()) {
        retVal = QUrl::fromLocalFile(m_path.absoluteFilePath(m_qmlFileName));
    }
    return retVal;
}

QUrl QmlMetadata::vuiFilePath() const
{
    QUrl retVal = QUrl();
    if (!m_vuiFileName.isEmpty()) {
        retVal = QUrl::fromLocalFile(m_path.absoluteFilePath(m_vuiFileName));
    }
    return retVal;
}

void QmlMetadata::setIconFileName(const QUrl &fileName)
{
    if (fileName.isRelative()) {
        m_icon = QUrl::fromLocalFile(m_path.absoluteFilePath(fileName.toLocalFile()));
    } else {
        m_icon = fileName;
    }
}

void QmlMetadata::setIsAudio(bool isAudio)
{
    m_isAudio = isAudio;
    emit changed();
}

void QmlMetadata::setIsHidden(bool isHidden)
{
    m_isHidden = isHidden;
    emit changed();
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
    emit changed();
}

void QmlMetadata::setGpuAlt(const QString &gpuAlt)
{
    m_gpuAlt = gpuAlt;
    emit changed();
}

void QmlMetadata::setAllowMultiple(bool allowMultiple)
{
    m_allowMultiple = allowMultiple;
}

void QmlMetadata::setIsClipOnly(bool isClipOnly)
{
    m_isClipOnly = isClipOnly;
}

bool QmlMetadata::isMltVersion(const QString &version)
{
    if (!m_minimumVersion.isEmpty()) {
        LOG_DEBUG() << "MLT version:" << version << "Shotcut minimumVersion:" << m_minimumVersion;
        if (QVersionNumber::fromString(version) < QVersionNumber::fromString(m_minimumVersion))
            return false;
    }
    return true;
}

QmlKeyframesMetadata::QmlKeyframesMetadata(QObject *parent)
    : QObject(parent)
    , m_allowTrim(true)
    , m_allowAnimateIn(false)
    , m_allowAnimateOut(false)
    , m_enabled(true)
    , m_allowSmooth(true)
{
}

void QmlKeyframesMetadata::checkVersion(const QString &version)
{
    if (!m_minimumVersion.isEmpty()) {
        LOG_DEBUG() << "MLT version:" << version << "Shotcut minimumVersion:" << m_minimumVersion;
        if (QVersionNumber::fromString(version) < QVersionNumber::fromString(m_minimumVersion))
            setDisabled();
    }
}

void QmlKeyframesMetadata::setDisabled()
{
    m_enabled = m_allowAnimateIn = m_allowAnimateOut = false;
}

QmlKeyframesParameter::QmlKeyframesParameter(QObject *parent)
    : QObject(parent)
    , m_isCurve(false)
    , m_minimum(0.0)
    , m_maximum(0.0)
    , m_units("")
    , m_isRectangle(false)
    , m_rangeType(MinMax)
{
}
