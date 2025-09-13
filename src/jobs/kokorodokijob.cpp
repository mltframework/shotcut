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

#include "kokorodokijob.h"

#include "Logger.h"
#include "jobqueue.h"
#include "jobs/dockerpulljob.h"
#include "mainwindow.h"
#include "qmltypes/qmlapplication.h"
#include "settings.h"
#include "util.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>

static const auto kDockerImageRef = QStringLiteral("mltframework/kokorodoki");

KokorodokiJob::KokorodokiJob(const QString &inputFile,
                             const QString &outputFile,
                             const QString &language,
                             const QString &voice,
                             double speed,
                             QThread::Priority priority)
    : AbstractJob(QObject::tr("Text to speech: %1").arg(QFileInfo(outputFile).fileName()), priority)
    , m_inputFile(inputFile)
    , m_outputFile(outputFile)
    , m_language(language)
    , m_voice(voice)
    , m_speed(speed)
{
    setTarget(outputFile);
    QAction *action = new QAction(tr("Open"), this);
    action->setData("Open");
    connect(action, &QAction::triggered, this, [this]() { onOpenTriggered(); });
    m_successActions << action;
}

KokorodokiJob::~KokorodokiJob()
{
    LOG_DEBUG() << "KokorodokiJob destroyed";
}

bool KokorodokiJob::checkDockerImage(QWidget *parent)
{
    auto dockerKokorodokiExists = Util::dockerStatus("mltframework/kokorodoki");
    if (!dockerKokorodokiExists.first) {
        QMessageBox
            dialog(QMessageBox::Warning,
                   QApplication::applicationName(),
                   parent
                       ->tr("<p>This feature requires <b><a "
                            "href=\"https://www.docker.com/\">Docker</a></b>, which provides an "
                            "installer, and the automatic download of an <b><big>13.2 GB</big></b> "
                            "Docker image.</p><p>If you already installed Docker it could not be "
                            "found at the expected location: <tt>%1</tt></p><p>Click <b>OK</b> to "
                            "continue and locate the <tt>docker</tt> program on your system.</p>")
                       .arg(Settings.dockerPath()),
                   QMessageBox::Cancel | QMessageBox::Ok,
                   parent);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Ok);
        dialog.setEscapeButton(QMessageBox::Cancel);
        if (QMessageBox::Cancel == dialog.exec())
            return false;
    } else if (!dockerKokorodokiExists.second) {
        QMessageBox
            dialog(QMessageBox::Question,
                   QApplication::applicationName(),
                   parent
                       ->tr("<p>This feature requires the automatic download of an <b><big>13.2 "
                            "GB</big></b> Docker image.</p><p>Do you want to continue?</p>")
                       .arg(Settings.dockerPath()),
                   QMessageBox::No | QMessageBox::Yes,
                   parent);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        if (QMessageBox::No == dialog.exec())
            return false;
    }
    return true;
}

// static
void KokorodokiJob::prepareAndRun(QWidget *parent, std::function<void()> callback)
{
    if (!callback)
        return;
    Util::isDockerImageCurrentAsync(kDockerImageRef, parent, [callback](bool current) {
        LOG_DEBUG() << "dockerImageIsCurrent" << current;
        if (current) {
            callback();
        } else {
            auto pullJob = new DockerPullJob(kDockerImageRef);
            QObject::connect(pullJob,
                             &AbstractJob::finished,
                             pullJob,
                             [callback](AbstractJob *, bool success) {
                                 if (success)
                                     callback();
                             });
            JOBS.add(pullJob);
        }
    });
}

void KokorodokiJob::start()
{
    QFileInfo inFi(m_inputFile);
    QFileInfo outFi(m_outputFile);
    if (inFi.dir() != outFi.dir()) {
        // For safety ensure same directory
        LOG_INFO() << "Input and output not in same directory; aborting.";
        emit progressUpdated(m_item, 0);
        kill();
        return;
    }

    auto docker = Settings.dockerPath();
    auto dirPath = outFi.dir().absolutePath();
    auto baseIn = inFi.fileName();
    auto baseOut = outFi.fileName();

    QStringList args;
    args << "run"
         << "--cpus" << QString::number(std::max(1, QThread::idealThreadCount() - 1)) << "-t"
         << "-e"
         << "SKIP_STARTUP_MSG=1";
    args << "-v" << QStringLiteral("%1:/mnt").arg(dirPath);
    args << "-w"
         << "/mnt";
    args << "--rm";
    args << "mltframework/kokorodoki";
    args << "-l" << m_language;
    args << "-v" << m_voice;
    args << "-s" << QString::number(m_speed, 'f', 2);
    args << "-f" << baseIn;
    args << "-o" << baseOut;

    setReadChannel(QProcess::StandardOutput);
    setProcessChannelMode(QProcess::MergedChannels);

    LOG_DEBUG() << docker + " " + args.join(' ');
    AbstractJob::start(docker, args);
    emit progressUpdated(m_item, 0);
}

void KokorodokiJob::onReadyRead()
{
    QString line;
    static QRegularExpression timeRe("(\\d{1,2}):(\\d{2}):(\\d{2})\\b");
    static QRegularExpression ansiRe("\x1B\[[0-9;?]*[A-Za-z]");
    // Any Braille pattern character (U+2800–U+28FF) often used by rich.console spinners.
    static QRegularExpression brailleRe("[\u2800-\u28FF]");
    do {
        line = readLine();
        if (!line.isEmpty()) {
            // Remove ANSI escape sequences (color/spinner control, etc.).
            line.replace(ansiRe, "");
            // Remove braille spinner characters.
            line.replace(brailleRe, "");
            // Trim space left after spinner removal.
            line = line.trimmed();
            // Suppress duplicate consecutive lines.
            if (line == m_lastLogLine) {
                continue;
            }
            m_lastLogLine = line;
            appendToLog(line.append('\n'));
            // Very rough progress: update percent based on elapsed time relative to last seen.
            const auto match = timeRe.match(line);
            if (match.hasMatch()) {
                const auto h = match.captured(1);
                const auto m = match.captured(2);
                const auto s = match.captured(3);
                const auto tc = h + ":" + m + ":" + s;
                if (tc != m_lastTimecode) {
                    m_lastTimecode = tc;
                    // Convert to seconds; no known total so just map seconds to a capped percent.
                    auto seconds = h.toInt() * 3600 + m.toInt() * 60 + s.toInt();
                    emit progressUpdated(m_item, std::min(99, 3 + seconds));
                }
            }
            if (line.contains("Kokoro initialized"))
                emit progressUpdated(m_item, 2);
            else if (line.contains("pipeline initialized"))
                emit progressUpdated(m_item, 3);
            else if (line.contains("Saved to "))
                emit progressUpdated(m_item, 100);
        }
    } while (!line.isEmpty());
}

void KokorodokiJob::onOpenTriggered()
{
    MAIN.open(m_outputFile);
}
