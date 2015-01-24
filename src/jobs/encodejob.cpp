/*
 * Copyright (c) 2012-2015 Meltytech, LLC
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

#include "encodejob.h"
#include <QAction>
#include <QUrl>
#include <QDesktopServices>
#include <QFileInfo>
#include "mainwindow.h"

EncodeJob::EncodeJob(const QString &name, const QString &xml)
    : MeltJob(name, xml)
{
    QAction* action = new QAction(tr("Open"), this);
    action->setToolTip(tr("Open the output file in the Shotcut player"));
    connect(action, &QAction::triggered, this, &EncodeJob::onOpenTiggered);
    m_successActions << action;

    action = new QAction(tr("Show In Folder"), this);
    action->setToolTip(tr("Show In Folder"));
    connect(action, &QAction::triggered, this, &EncodeJob::onShowFolderTriggered);
    m_successActions << action;
}

void EncodeJob::onOpenTiggered()
{
    MAIN.open(objectName().toUtf8().constData());
}

void EncodeJob::onShowFolderTriggered()
{
    QFileInfo fi(objectName());
    QUrl url(QString("file://").append(fi.path()), QUrl::TolerantMode);
    QDesktopServices::openUrl(url);
}
