/*
 * Copyright (c) 2012-2022 Meltytech, LLC
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

#include "videoqualityjob.h"
#include <QAction>
#include <QFile>
#include <QDomDocument>
#include <QTextStream>
#include <QFileInfo>
#include <QUrl>
#include <QDesktopServices>
#include "mainwindow.h"
#include "dialogs/textviewerdialog.h"
#include "util.h"

VideoQualityJob::VideoQualityJob(const QString &name, const QString &xml,
                                 const QString &reportPath, int frameRateNum, int frameRateDen)
    : MeltJob(name, xml, frameRateNum, frameRateDen)
    , m_reportPath(reportPath)
{
    QAction *action = new QAction(tr("Open"), this);
    action->setData("Open");
    action->setToolTip(tr("Open original and encoded side-by-side in the Shotcut player"));
    connect(action, SIGNAL(triggered()), this, SLOT(onOpenTiggered()));
    m_successActions << action;

    action = new QAction(tr("View Report"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onViewReportTriggered()));
    m_successActions << action;

    action = new QAction(tr("Show In Folder"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onShowFolderTriggered()));
    m_successActions << action;

    setLabel(tr("Measure %1").arg(objectName()));
    setStandardOutputFile(reportPath);
}

void VideoQualityJob::onOpenTiggered()
{
    // Parse the XML.
    QFile file(xmlPath());
    file.open(QIODevice::ReadOnly);
    QDomDocument dom(xmlPath());
    dom.setContent(&file);
    file.close();

    // Locate the VQM transition.
    QDomNodeList transitions = dom.elementsByTagName("transition");
    for (int i = 0; i < transitions.length(); i++ ) {
        QDomElement property = transitions.at(i).firstChildElement("property");
        while (!property.isNull()) {
            // Change the render property to 1.
            if (property.attribute("name") == "render") {
                property.firstChild().setNodeValue("1");

                // Save the new XML.
                file.open(QIODevice::WriteOnly);
                QTextStream textStream(&file);
                dom.save(textStream, 2);
                file.close();

                MAIN.open(xmlPath().toUtf8().constData());
                break;
            }
            property = property.nextSiblingElement("property");
        }
    }
}

void VideoQualityJob::onViewReportTriggered()
{
    TextViewerDialog dialog(&MAIN);
    dialog.setWindowTitle(tr("Video Quality Measurement"));
    QFile f(m_reportPath);
    f.open(QIODevice::ReadOnly);
    QString s(f.readAll());
    f.close();
    dialog.setText(s);
    dialog.exec();
}
