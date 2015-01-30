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
#include <QFileDialog>
#include <QTemporaryFile>
#include <QDir>
#include <QDomDocument>
#include <QTextStream>
#include "mainwindow.h"
#include "settings.h"
#include "jobqueue.h"
#include "jobs/videoqualityjob.h"

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

    action = new QAction(tr("Measure Video Quality..."), this);
    connect(action, &QAction::triggered, this, &EncodeJob::onVideoQualityTriggered);
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

void EncodeJob::onVideoQualityTriggered()
{
    // Get the location and file name for the report.
    QString directory = Settings.encodePath();
    directory += "/.txt";
    QString reportPath= QFileDialog::getSaveFileName(&MAIN, tr("Video Quality Report"), directory);
    if (!reportPath.isEmpty()) {
        QFileInfo fi(reportPath);
        if (fi.suffix().isEmpty())
            reportPath += ".txt";

        // Get temp filename for the new XML.
        QTemporaryFile tmp(QDir::tempPath().append("/shotcut-XXXXXX"));
        tmp.open();
        QString tmpName = tmp.fileName();
        tmp.close();
        tmpName.append(".mlt");

        // Generate the XML for the comparison.
        Mlt::Tractor tractor(MLT.profile());
        Mlt::Producer original(MLT.profile(), xmlPath().toUtf8().constData());
        Mlt::Producer encoded(MLT.profile(), objectName().toUtf8().constData());
        Mlt::Transition vqm(MLT.profile(), "vqm");
        if (original.is_valid() && encoded.is_valid() && vqm.is_valid()) {
            tractor.set_track(original, 0);
            tractor.set_track(encoded, 1);
            tractor.plant_transition(vqm);
            vqm.set("render", 0);
            MLT.saveXML(tmpName, &tractor);

            // Add consumer element to XML.
            QFile f1(tmpName);
            f1.open(QIODevice::ReadOnly);
            QDomDocument dom(tmpName);
            dom.setContent(&f1);
            f1.close();

            QDomElement consumerNode = dom.createElement("consumer");
            QDomNodeList profiles = dom.elementsByTagName("profile");
            if (profiles.isEmpty())
                dom.documentElement().insertAfter(consumerNode, dom.documentElement());
            else
                dom.documentElement().insertAfter(consumerNode, profiles.at(profiles.length() - 1));
            consumerNode.setAttribute("mlt_service", "null");
            consumerNode.setAttribute("real_time", -1);
            consumerNode.setAttribute("terminate_on_pause", 1);

            // Save the new XML.
            f1.open(QIODevice::WriteOnly);
            QTextStream ts(&f1);
            dom.save(ts, 2);
            f1.close();

            // Create job and add it to the queue.
            JOBS.add(new VideoQualityJob(objectName(), tmpName, reportPath));
        }
    }
}
