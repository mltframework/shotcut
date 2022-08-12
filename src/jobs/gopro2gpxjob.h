/*
 * Copyright (c) 2022 Meltytech, LLC
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

#ifndef GOPRO2GPXJOB_H
#define GOPRO2GPXJOB_H

#include "abstractjob.h"
#include <QStringList>

class GoPro2GpxJob : public AbstractJob
{
    Q_OBJECT
public:
    GoPro2GpxJob(const QString &name, const QStringList &args);
    virtual ~GoPro2GpxJob() {};
    void start();

private:
    QStringList m_args;
};

#endif // GOPRO2GPXJOB_H
