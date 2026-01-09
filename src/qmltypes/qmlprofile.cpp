/*
 * Copyright (c) 2014 Meltytech, LLC
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

QmlProfile::QmlProfile()
    : QObject()
{}

int QmlProfile::width() const
{
    return MLT.profile().width();
}

int QmlProfile::height() const
{
    return MLT.profile().height();
}

double QmlProfile::aspectRatio() const
{
    return MLT.profile().dar();
}

double QmlProfile::fps() const
{
    return MLT.profile().fps();
}

double QmlProfile::sar() const
{
    return MLT.profile().sar();
}
