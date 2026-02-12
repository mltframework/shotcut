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

#include "encodejob.h"

#include "dialogs/listselectiondialog.h"
#include "docks/timelinedock.h"
#include "jobqueue.h"
#include "jobs/videoqualityjob.h"
#include "mainwindow.h"
#include "qmltypes/qmlapplication.h"
#include "qmltypes/qmlutilities.h"
#include "settings.h"
#include "spatialmedia/spatialmedia.h"
#include "util.h"

#include <QAction>
#include <QDesktopServices>
#include <QDir>
#include <QDomDocument>
#include <QFileDialog>
#include <QFileInfo>
#include <QJSEngine>
#include <QTemporaryFile>
#include <QTextStream>
#include <QUrl>

#include "Logger.h"

EncodeJob::EncodeJob(const QString &name,
                     const QString &xml,
                     int frameRateNum,
                     int frameRateDen,
                     QThread::Priority priority)
    : MeltJob(name, xml, frameRateNum, frameRateDen, priority)
{
    QAction *action = new QAction(tr("Open"), this);
    action->setData("Open");
    action->setToolTip(tr("Open the output file in the Shotcut player"));
    connect(action, SIGNAL(triggered()), this, SLOT(onOpenTiggered()));
    m_successActions << action;

    action = new QAction(tr("Show In Files"), this);
    action->setToolTip(tr("Show In Files"));
    connect(action, SIGNAL(triggered()), this, SLOT(onShowInFilesTriggered()));
    m_successActions << action;

    action = new QAction(tr("Show In Folder"), this);
    action->setToolTip(tr("Show In Folder"));
    connect(action, SIGNAL(triggered()), this, SLOT(onShowFolderTriggered()));
    m_successActions << action;

    action = new QAction(tr("Measure Video Quality..."), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onVideoQualityTriggered()));
    m_successActions << action;

    action = new QAction(tr("Set Equirectangular..."), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onSpatialMediaTriggered()));
    m_successActions << action;

    action = new QAction(tr("Embed Markers as Chapters..."), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onEmbedChapters()));
    m_successActions << action;
}

void EncodeJob::onVideoQualityTriggered()
{
    // Get the location and file name for the report.
    QString directory = Settings.encodePath();
    QString caption = tr("Video Quality Report");
    QString nameFilter = tr("Text Documents (*.txt);;All Files (*)");
    QString reportPath = QFileDialog::getSaveFileName(&MAIN,
                                                      caption,
                                                      directory,
                                                      nameFilter,
                                                      nullptr,
                                                      Util::getFileDialogOptions());
    if (!reportPath.isEmpty()) {
        QFileInfo fi(reportPath);
        if (fi.suffix().isEmpty())
            reportPath += ".txt";

        if (Util::warnIfNotWritable(reportPath, &MAIN, caption))
            return;

        // Get temp file for the new XML.
        QScopedPointer<QTemporaryFile> tmp(Util::writableTemporaryFile(reportPath));
        if (!tmp->open()) {
            LOG_ERROR() << "Failed to create temporary file" << tmp->fileName();
            return;
        }

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
            MLT.saveXML(tmp->fileName(), &tractor, false /* without relative paths */, tmp.data());
            tmp->close();

            // Add consumer element to XML.
            QFile f1(tmp->fileName());
            if (!f1.open(QIODevice::ReadOnly)) {
                LOG_ERROR() << "Failed to open XML file for reading" << tmp->fileName();
                return;
            }
            QDomDocument dom(tmp->fileName());
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

            // Create job and add it to the queue.
            JOBS.add(new VideoQualityJob(objectName(),
                                         dom.toString(2),
                                         reportPath,
                                         MLT.profile().frame_rate_num(),
                                         MLT.profile().frame_rate_den()));
        }
    }
}

void EncodeJob::onSpatialMediaTriggered()
{
    // Get the location and file name for the output file.
    QString caption = tr("Set Equirectangular Projection");
    QFileInfo info(objectName());
    QString directory = QStringLiteral("%1/%2 - ERP.%3")
                            .arg(Settings.encodePath())
                            .arg(info.completeBaseName())
                            .arg(info.suffix());
    QString filePath = QFileDialog::getSaveFileName(&MAIN,
                                                    caption,
                                                    directory,
                                                    QString(),
                                                    nullptr,
                                                    Util::getFileDialogOptions());
    if (!filePath.isEmpty()) {
        if (SpatialMedia::injectSpherical(objectName().toStdString(), filePath.toStdString())) {
            MAIN.showStatusMessage(tr("Successfully wrote %1").arg(QFileInfo(filePath).fileName()));
        } else {
            MAIN.showStatusMessage(tr("An error occurred saving the projection."));
        }
    }
}

void EncodeJob::onEmbedChapters()
{
    // Options dialog
    auto uniqueColors = MAIN.timelineDock()->markersModel()->allColors();
    if (uniqueColors.isEmpty()) {
        return;
    }
    std::sort(uniqueColors.begin(), uniqueColors.end(), [=](const QColor &a, const QColor &b) {
        if (a.hue() == b.hue()) {
            if (a.saturation() == b.saturation()) {
                return a.value() <= b.value();
            }
            return a.saturation() <= b.saturation();
        }
        return a.hue() <= b.hue();
    });
    QStringList colors;
    for (auto &color : uniqueColors) {
        colors << color.name();
    }
    const auto rangesOption = tr("Include ranges (Duration > 1 frame)?");
    QStringList initialOptions;
    for (auto &m : MAIN.timelineDock()->markersModel()->getMarkers()) {
        if (m.end != m.start) {
            initialOptions << rangesOption;
            break;
        }
    }

    ListSelectionDialog dialog(initialOptions, &MAIN);
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.setWindowTitle(tr("Choose Markers"));
    if (Settings.exportRangeMarkers()) {
        dialog.setSelection({rangesOption});
    }
    dialog.setColors(colors);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto selection = dialog.selection();
    Settings.setExportRangeMarkers(selection.contains(rangesOption));

    // Get the location and file name for the output file.
    QString caption = tr("Embed Chapters");
    QFileInfo info(objectName());
    QString directory = QStringLiteral("%1/%2 - Chapters.%3")
                            .arg(Settings.encodePath(), info.completeBaseName(), info.suffix());
    QString filePath = QFileDialog::getSaveFileName(&MAIN,
                                                    caption,
                                                    directory,
                                                    QString(),
                                                    nullptr,
                                                    Util::getFileDialogOptions());
    if (!filePath.isEmpty()) {
        if (Util::warnIfNotWritable(filePath, &MAIN, caption))
            return;

        // Locate the JavaScript file in the filesystem.
        QDir qmlDir = QmlUtilities::qmlDir();
        qmlDir.cd("export-chapters");
        auto jsFileName = qmlDir.absoluteFilePath("export-chapters.js");
        QFile scriptFile(jsFileName);
        if (scriptFile.open(QIODevice::ReadOnly)) {
            // Read JavaScript into a string.
            QTextStream stream(&scriptFile);
            stream.setEncoding(QStringConverter::Utf8);
            stream.setAutoDetectUnicode(true);
            QString contents = stream.readAll();
            scriptFile.close();

            // Evaluate JavaScript.
            QJSEngine jsEngine;
            QJSValue result = jsEngine.evaluate(contents, jsFileName);
            if (!result.isError()) {
                // Call the JavaScript main function.
                QJSValue options = jsEngine.newObject();
                options.setProperty("ffmetadata", true);
                if (selection.contains(rangesOption)) {
                    options.setProperty("includeRanges", true);
                    selection.removeOne(rangesOption);
                }
                QJSValue array = jsEngine.newArray(selection.size());
                for (int i = 0; i < selection.size(); ++i)
                    array.setProperty(i, selection[i].toUpper());
                options.setProperty("colors", array);
                QJSValueList args;
                args << MLT.XML(0, true, true) << options;
                result = result.call(args);
                if (!result.isError()) {
                    // Save the result with the export file name.
                    auto tempFile = Util::writableTemporaryFile(filePath, "shotcut-XXXXXX.txt");
                    tempFile->write(result.toString().toUtf8());
                    tempFile->close();

                    QStringList args;
                    args << "-i" << objectName() << "-i" << tempFile->fileName() << "-map_metadata"
                         << "1"
                         << "-c"
                         << "copy"
                         << "-y" << filePath;
                    auto job = new FfmpegJob(filePath, args, false);
                    job->setLabel(filePath);
                    tempFile->setParent(job);
                    JOBS.add(job);
                }
            } else {
                LOG_ERROR() << "Uncaught exception at line" << result.property("lineNumber").toInt()
                            << ":" << result.toString();
                MAIN.showStatusMessage(tr("A JavaScript error occurred during export."));
            }
        } else {
            MAIN.showStatusMessage(tr("Failed to open export-chapters.js"));
        }
    }
}

void EncodeJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus != QProcess::NormalExit && exitCode != 0 && !stopped()) {
        LOG_INFO() << "job failed with" << exitCode;
        appendToLog(QStringLiteral("Failed with exit code %1\n").arg(exitCode));
        bool isParallel = false;
        // Parse the XML.
        if (!m_xml->open()) {
            LOG_ERROR() << "Failed to open XML file for reading" << m_xml->fileName();
            return;
        }
        QDomDocument dom(xmlPath());
        dom.setContent(m_xml.data());
        m_xml->close();

        // Locate the consumer element.
        QDomNodeList consumers = dom.elementsByTagName("consumer");
        for (int i = 0; i < consumers.length(); i++) {
            QDomElement consumer = consumers.at(i).toElement();
            // If real_time is set for parallel.
            if (consumer.attribute("real_time").toInt() < -1) {
                isParallel = true;
                consumer.setAttribute("real_time", "-1");
            }
        }
        if (isParallel) {
            QString message(tr("Export job failed; trying again without Parallel processing."));
            MAIN.showStatusMessage(message);
            appendToLog(message.append("\n"));
            if (!m_xml->open()) {
                LOG_ERROR() << "Failed to open XML file for reading" << m_xml->fileName();
                return;
            }
            QTextStream textStream(m_xml.data());
            dom.save(textStream, 2);
            m_xml->close();
            MeltJob::start();
            return;
        }
    }
    MeltJob::onFinished(exitCode, exitStatus);
}
