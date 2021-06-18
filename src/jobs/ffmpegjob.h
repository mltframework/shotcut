/*
 * Copyright (c) 2016-2019 Meltytech, LLC
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

#ifndef FFMPEGJOB_H
#define FFMPEGJOB_H

#include "abstractjob.h"
#include <QStringList>

class FfmpegJob : public AbstractJob
{
    Q_OBJECT
public:
    FfmpegJob(const QString& name, const QStringList& args, bool isOpenLog = true);
    virtual ~FfmpegJob();
    void start();

private slots:
    void onOpenTriggered();
    void onReadyRead();

private:
    QStringList m_args;
    QString m_duration;
    bool m_outputMsgRead;
    int m_totalFrames;
    int m_previousPercent;
    bool m_isOpenLog;
};

#endif // FFMPEGJOB_H
