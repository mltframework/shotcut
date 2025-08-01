/*
 * Copyright (c) 2012-2025 Meltytech, LLC
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

#ifndef ENCODEJOB_H
#define ENCODEJOB_H

#include "meltjob.h"

#include <QThread>

class EncodeJob : public MeltJob
{
    Q_OBJECT
public:
    EncodeJob(const QString &name,
              const QString &xml,
              int frameRateNum,
              int frameRateDen,
              const QThread::Priority priority);

private slots:
    void onVideoQualityTriggered();
    void onSpatialMediaTriggered();
    void onEmbedChapters();

protected slots:
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // ENCODEJOB_H
