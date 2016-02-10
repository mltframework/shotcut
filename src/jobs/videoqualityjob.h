/*
 * Copyright (c) 2012-2016 Meltytech, LLC
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

#ifndef VIDEOQUALITYJOB_H
#define VIDEOQUALITYJOB_H

#include "meltjob.h"

class VideoQualityJob : public MeltJob
{
    Q_OBJECT
public:
    VideoQualityJob(const QString& name, const QString& xml,
                    const QString& reportPath);

private slots:
    void onOpenTiggered();
    void onViewReportTriggered();
    void onShowFolderTriggered();

private:
    QString m_reportPath;
};

#endif // VIDEOQUALITYJOB_H
