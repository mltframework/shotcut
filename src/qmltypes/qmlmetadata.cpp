/*
 * Copyright (c) 2013-2026 Meltytech, LLC
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

#include "Logger.h"
#include "settings.h"

#include <QVersionNumber>

/*!
    \qmltype Metadata
    \inqmlmodule org.shotcut.qml
    \brief Describes a filter's identity, capabilities, and keyframe configuration.

    \c Metadata is the read-only descriptor for a filter service. It is available
    in \b Filters panels as the \c metadata context property. Use it to inspect
    the filter type, supported capabilities, and keyframe parameters.
*/

/*!
    \qmlproperty int Metadata::type
    \brief The plugin type.
    One of \c Filter (0), \c Producer (1), \c Transition (2), \c Link (3), or \c FilterSet (4).
*/

/*!
    \qmlproperty string Metadata::name
    \brief The human-readable display name of the filter (e.g. \c "Blur").
*/

/*!
    \qmlproperty string Metadata::mlt_service
    \brief The MLT service identifier (e.g. \c "avfilter.boxblur").
*/

/*!
    \qmlproperty bool Metadata::needsGPU
    \brief Whether this filter requires GPU (GLSL) processing.
*/

/*!
    \qmlproperty string Metadata::qml
    \brief The file name of the filter's User Interface (UI) parameter panel (relative to the filter directory).
*/

/*!
    \qmlproperty string Metadata::vui
    \brief The file name of the filter's VUI overlay QML file (relative to the filter directory).
*/

/*!
    \qmlproperty bool Metadata::isAudio
    \brief Whether this filter operates on audio rather than video.
*/

/*!
    \qmlproperty bool Metadata::isHidden
    \brief Whether this filter is hidden from the filter list UI.
*/

/*!
    \qmlproperty bool Metadata::isFavorite
    \brief Whether the user has marked this filter as a favorite.
*/

/*!
    \qmlproperty bool Metadata::allowMultiple
    \brief Whether multiple instances of this filter can be applied to the same clip.
*/

/*!
    \qmlproperty bool Metadata::isClipOnly
    \brief Whether this filter can only be applied to clips (not tracks or timeline output).
*/

/*!
    \qmlproperty bool Metadata::isTrackOnly
    \brief Whether this filter can only be applied to tracks.
*/

/*!
    \qmlproperty bool Metadata::isOutputOnly
    \brief Whether this filter can only be applied to the timeline output.
*/

/*!
    \qmlproperty bool Metadata::isDeprecated
    \brief Whether this filter has been deprecated and should not be used in new projects.
*/

/*!
    \qmlproperty string Metadata::minimumVersion
    \brief The minimum MLT metadata version required for this filter.
*/

/*!
    \qmlproperty string Metadata::keywords
    \brief Space-separated keywords for searching this filter in the filter list.
*/

/*!
    \qmlproperty string Metadata::help
    \brief A link to the documentation page for this filter.
*/

/*!
    \qmlproperty bool Metadata::seekReverse
    \brief Whether this filter may need to repeatedly perform reverse seeking for certain \b Time filters.
*/

/*!
    \qmlproperty KeyframesMetadata Metadata::keyframes
    \brief The keyframes configuration object for this filter.
    \sa KeyframesMetadata
*/

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
    , m_isTrackOnly(false)
    , m_isOutputOnly(false)
    , m_isGpuCompatible(true)
    , m_isDeprecated(false)
    , m_seekReverse(false)
{}

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
    } else if (m_type == FilterSet) {
        return m_name;
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

/*!
    \qmlproperty url Metadata::qmlFilePath
    \brief The full URL to the filter's QML parameter panel file.
*/

QUrl QmlMetadata::qmlFilePath() const
{
    QUrl retVal = QUrl();
    if (!m_qmlFileName.isEmpty()) {
        retVal = QUrl::fromLocalFile(m_path.absoluteFilePath(m_qmlFileName));
    }
    return retVal;
}

/*!
    \qmlproperty url Metadata::vuiFilePath
    \brief The full URL to the filter's VUI overlay file.
*/

QUrl QmlMetadata::vuiFilePath() const
{
    QUrl retVal = QUrl();
    if (!m_vuiFileName.isEmpty()) {
        retVal = QUrl::fromLocalFile(m_path.absoluteFilePath(m_vuiFileName));
    }
    return retVal;
}

void QmlMetadata::setIconFileName(const QString &fileName)
{
    m_icon = fileName;
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

void QmlMetadata::setIsTrackOnly(bool isTrackOnly)
{
    m_isTrackOnly = isTrackOnly;
}

void QmlMetadata::setIsOutputOnly(bool isOutputOnly)
{
    m_isOutputOnly = isOutputOnly;
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

/*!
    \qmltype KeyframesMetadata
    \inqmlmodule org.shotcut.qml
    \brief Describes the keyframe capabilities of a filter.

    Accessed via \l{Metadata::keyframes}. Describes whether the filter supports
    trim keyframes, animate-in/out ramps, and which parameters are keyframeable.
*/

/*!
    \qmlproperty bool KeyframesMetadata::allowTrim
    \brief Whether the filter's in/out points can be trimmed to control when it is active.
*/

/*!
    \qmlproperty bool KeyframesMetadata::allowAnimateIn
    \brief Whether the filter supports an "animate in" simple keyframe ramp.
*/

/*!
    \qmlproperty bool KeyframesMetadata::allowAnimateOut
    \brief Whether the filter supports an "animate out" simple keyframe ramp.
*/

/*!
    \qmlproperty list<Parameter> KeyframesMetadata::parameters
    \brief The list of keyframeable parameters for this filter.
    \sa Parameter
*/

/*!
    \qmlproperty list<string> KeyframesMetadata::simpleProperties
    \brief The list of MLT property names that use the simple (two-keyframe) animation mode.
*/

/*!
    \qmlproperty string KeyframesMetadata::minimumVersion
    \brief The minimum MLT metadata version required for full keyframe support of this filter.
*/

/*!
    \qmlproperty bool KeyframesMetadata::enabled
    \brief Whether keyframe support is enabled for this filter at all.
*/

/*!
    \qmlproperty bool KeyframesMetadata::allowOvershoot
    \brief Whether keyframe interpolation is allowed to overshoot the min/max parameter range.
*/

QmlKeyframesMetadata::QmlKeyframesMetadata(QObject *parent)
    : QObject(parent)
    , m_allowTrim(true)
    , m_allowAnimateIn(false)
    , m_allowAnimateOut(false)
    , m_enabled(true)
    , m_allowOvershoot(true)
{}

QmlKeyframesParameter *QmlKeyframesMetadata::parameter(const QString &propertyName) const
{
    for (const auto &p : m_parameters) {
        if (propertyName == p->property())
            return p;
    }
    return nullptr;
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

/*!
    \qmltype Parameter
    \inqmlmodule org.shotcut.qml
    \brief Describes a single keyframeable parameter within a filter's keyframes metadata.

    Accessed from \l{KeyframesMetadata::parameters}.
*/

/*!
    \qmlproperty int Parameter::rangeType
    \brief The range constraint type. \c 0 = MinMax (clamped to \l minimum / \l maximum),
    \c 1 = ClipLength (range is 0 to clip duration in frames).
*/

/*!
    \qmlproperty string Parameter::name
    \brief The human-readable display name of the parameter.
*/

/*!
    \qmlproperty string Parameter::property
    \brief The MLT property name this parameter maps to (used with \l{Filter::get()} and \l{Filter::set()}).
*/

/*!
    \qmlproperty list<string> Parameter::gangedProperties
    \brief Additional MLT property names that must be adjusted together with \l property
    (e.g. linked x/y coordinates).
*/

/*!
    \qmlproperty list<string> Parameter::gangedRectProperties
    \brief Additional rect-type MLT property names that must be adjusted together with \l property.
*/

/*!
    \qmlproperty bool Parameter::isCurve
    \brief Whether this parameter can display a curve editor in the \b Keyframes panel.
*/

/*!
    \qmlproperty real Parameter::minimum
    \brief The minimum allowed value when \l rangeType is \c MinMax.
*/

/*!
    \qmlproperty real Parameter::maximum
    \brief The maximum allowed value when \l rangeType is \c MinMax.
*/

/*!
    \qmlproperty string Parameter::units
    \brief The display units string shown next to the parameter value (e.g. \c "px", \c "%").
*/

/*!
    \qmlproperty bool Parameter::isRectangle
    \brief Whether this parameter represents a rectangle (uses \l{Filter::getRect()} / \l{Filter::set()}).
*/

/*!
    \qmlproperty bool Parameter::isColor
    \brief Whether this parameter represents a color (uses \l{Filter::getColor()} / \l{Filter::set()}).
*/

QmlKeyframesParameter::QmlKeyframesParameter(QObject *parent)
    : QObject(parent)
    , m_isCurve(false)
    , m_minimum(0.0)
    , m_maximum(0.0)
    , m_units("")
    , m_isRectangle(false)
    , m_rangeType(MinMax)
    , m_isColor(false)
{}
