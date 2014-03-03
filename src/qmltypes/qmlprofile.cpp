/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Brian Matherly <pez4brian@yahoo.com>
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

QmlProfile::QmlProfile(const Mlt::Profile& profile, QObject *parent)
    : QObject(parent)
    , m_profile(mlt_profile_clone(profile.get_profile()))
{
}

QmlProfile::~QmlProfile()
{
}

int QmlProfile::width() const
{
    return m_profile.width();
}

int QmlProfile::height() const
{
    return m_profile.height();
}
