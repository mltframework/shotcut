/*
 * Copyright (c) 2025 Meltytech, LLC
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

#ifndef DOCKERPULLJOB_H
#define DOCKERPULLJOB_H

#include "abstractjob.h"

#include <QString>

class DockerPullJob : public AbstractJob
{
    Q_OBJECT
public:
    DockerPullJob(const QString &imageRef, QThread::Priority priority = Settings.jobPriority());
    virtual ~DockerPullJob();

public slots:
    void start() override;

protected slots:
    void onReadyRead() override;

private:
    QString m_imageRef;
    int m_previousPercent{-1};
};

#endif // DOCKERPULLJOB_H
