/*
 * Copyright (c) 2023 Meltytech, LLC
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

#include "bitrateviewerjob.h"
#include "dialogs/bitratedialog.h"
#include "mainwindow.h"
#include "util.h"
#include "Logger.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

BitrateViewerJob::BitrateViewerJob(const QString &name, const QStringList &args, double fps)
    : FfprobeJob(name, args)
    , m_resource(args.last())
    , m_fps(fps)
{
    QAction *action = new QAction(tr("Open"), this);
    action->setData("Open");
    connect(action, &QAction::triggered, this, &BitrateViewerJob::onOpenTriggered);
    m_successActions << action;
}

BitrateViewerJob::~BitrateViewerJob()
{

}

void BitrateViewerJob::onFinished(int exitCode, ExitStatus exitStatus)
{
    AbstractJob::onFinished(exitCode, exitStatus);
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QJsonParseError error;
        auto s = log();
        s = s.left(s.lastIndexOf('}') + 1);
        auto doc = QJsonDocument::fromJson(s.toUtf8(), &error);
        if (QJsonParseError::NoError == error.error && doc.isObject()) {
            auto v = doc.object().value("packets");
            if (v.isArray()) {
                m_data = v.toArray();
                onOpenTriggered();
            }
        } else {
            LOG_ERROR() << "JSON parsing error:" << error.errorString();
        }
    }
}

void BitrateViewerJob::onOpenTriggered()
{
    BitrateDialog dialog(Util::baseName(m_resource), m_fps, m_data, &MAIN);
    dialog.exec();
}
