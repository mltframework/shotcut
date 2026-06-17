/*
 * Copyright (c) 2014-2026 Meltytech, LLC
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

#include "qmlprofile.h"

#include "mltcontroller.h"

QmlProfile &QmlProfile::singleton()
{
    static QmlProfile instance;
    return instance;
}

/*!
    \qmltype Profile
    \inqmlmodule org.shotcut.qml
    \brief Describes the current project's \b Video \b Mode, accessed via the \c profile context property.

    All properties reflect the active project profile.
    A \l profileChanged signal is emitted when the profile switches
    (e.g. when a new project is opened or the profile is explicitly changed).
    After receiving \l profileChanged, re-read all properties since they may have new values.

    \code
    var w = profile.width
    var h = profile.height
    var scale = profile.width / profile.height * profile.sar
    \endcode
*/

/*!
    \qmlsignal Profile::profileChanged()
    \brief Emitted when the project profile changes.
    Re-read all properties after receiving this signal.
*/

QmlProfile::QmlProfile()
    : QObject()
{}

/*!
    \qmlproperty int Profile::width
    \brief The frame width of the current video profile in pixels.
*/

int QmlProfile::width() const
{
    return MLT.profile().width();
}

/*!
    \qmlproperty int Profile::height
    \brief The frame height of the current video profile in pixels.
*/

int QmlProfile::height() const
{
    return MLT.profile().height();
}

/*!
    \qmlproperty real Profile::aspectRatio
    \brief The display aspect ratio (DAR = width / height × SAR) of the current video profile.
*/

double QmlProfile::aspectRatio() const
{
    return MLT.profile().dar();
}

/*!
    \qmlproperty real Profile::fps
    \brief The frames per second of the current video profile.
*/

double QmlProfile::fps() const
{
    return MLT.profile().fps();
}

/*!
    \qmlproperty real Profile::sar
    \brief The sample (pixel) aspect ratio of the current video profile.
    A value of 1.0 means square pixels.
*/

double QmlProfile::sar() const
{
    return MLT.profile().sar();
}
