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

#include "htmlgeneratorjob.h"

#include "Logger.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "settings.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryFile>

static double fps()
{
    return std::min(15.0, MLT.profile().fps());
}

HtmlGeneratorJob::HtmlGeneratorJob(const QString &name,
                                   const QString &html,
                                   const QString &outputPath,
                                   int duration,
                                   QThread::Priority priority)
    : AbstractJob(name, priority)
    , m_html(html)
    , m_outputPath(outputPath)
    , m_duration(duration)
    , m_generator(nullptr)
    , m_isGeneratingFrames(true)
    , m_previousPercent(0)
{
    setTarget(outputPath);

    // Create temporary directory for animation frames
    const auto outDir = QFileInfo(m_outputPath).dir();
    m_tempDir.reset(new QTemporaryDir(outDir.filePath("shotcut-htmlgen-XXXXXX")));
    if (!m_tempDir->isValid()) {
        LOG_ERROR() << "Failed to create temp directory for HTML animation frames:"
                    << m_tempDir->path();
    }

    auto *action = new QAction(tr("Open"), this);
    action->setData("Open");
    connect(action, &QAction::triggered, this, &HtmlGeneratorJob::onOpenTriggered);
    m_successActions << action;
}

void HtmlGeneratorJob::start()
{
    if (!m_tempDir || !m_tempDir->isValid()) {
        LOG_ERROR() << "Invalid temp directory";
        return;
    }

    // Create HTML file
    m_htmlFilePath = m_tempDir->path() + "/animation.html";
    QFile htmlFile(m_htmlFilePath);
    if (!htmlFile.open(QIODevice::WriteOnly)) {
        LOG_ERROR() << "Failed to create HTML file:" << htmlFile.errorString();
        return;
    }
    htmlFile.write(m_html.toUtf8());
    htmlFile.flush();
    htmlFile.close();

    // Create HTML generator for animation
    m_generator = new HtmlGenerator(this);

    // Set animation parameters: fps, duration in milliseconds
    m_generator->setAnimationParameters(fps(), m_duration);

    connect(m_generator,
            &HtmlGenerator::imageReady,
            this,
            &HtmlGeneratorJob::onAnimationFramesReady);
    connect(m_generator,
            &HtmlGenerator::progressUpdate,
            this,
            &HtmlGeneratorJob::onHtmlGeneratorProgress);

    const QString url("file://" + m_htmlFilePath);
    const QSize size(qRound(MLT.profile().width() * MLT.profile().sar()), MLT.profile().height());

    // Start the animation capture
    AbstractJob::start();
    m_isGeneratingFrames = true;

    m_generator->launchBrowser(Settings.chromiumPath(), url, size, m_tempDir->path());

    LOG_DEBUG() << "Started HTML animation generation:" << url;
}

void HtmlGeneratorJob::onAnimationFramesReady()
{
    LOG_DEBUG() << "Animation frames ready, starting FFmpeg conversion";
    m_isGeneratingFrames = false;
    emit progressUpdated(m_item, 80); // Frames generation is done

    // Clean up the generator
    if (m_generator) {
        m_generator->deleteLater();
        m_generator = nullptr;
    }

    // Now start FFmpeg process
    const auto shotcutPath = qApp->applicationDirPath();
    const QFileInfo ffmpegPath(shotcutPath, "ffmpeg");

    QStringList args;
    args << "-r" << QString::number(fps()) << "-i" << m_tempDir->path() + "/frame_%04d.png"
         << "-codec:v"
         << "utvideo"
         << "-pix_fmt"
         << "gbrap"
         << "-y" << m_outputPath;

    setReadChannel(QProcess::StandardError);
    LOG_DEBUG() << ffmpegPath.absoluteFilePath() + " " + args.join(' ');

    // Start FFmpeg process (this will call AbstractJob::start with program and args)
    AbstractJob::start(ffmpegPath.absoluteFilePath(), args);
}

void HtmlGeneratorJob::onReadyRead()
{
    if (m_isGeneratingFrames) {
        return; // Don't process FFmpeg output during frame generation
    }

    // Process FFmpeg output for progress reporting
    QString msg;
    do {
        msg = readLine();
        if (!msg.startsWith("frame=") && (!msg.trimmed().isEmpty())) {
            appendToLog(msg);
        }

        // Look for progress information in FFmpeg output
        if (msg.contains("frame=")) {
            // Extract frame number from FFmpeg output
            static QRegularExpression frameRegex("frame=\\s*(\\d+)");
            const auto match = frameRegex.match(msg);
            if (match.hasMatch()) {
                int currentFrame = match.captured(1).toInt();
                int totalFrames = qRound((m_duration / 1000.0) * fps());
                int percent = 80 + qRound((currentFrame * 80.0) / totalFrames); // 80-100%
                if (percent != m_previousPercent) {
                    emit progressUpdated(m_item, percent);
                    m_previousPercent = percent;
                }
            }
        }
    } while (!msg.isEmpty());
}

void HtmlGeneratorJob::onHtmlGeneratorProgress(float progress)
{
    if (m_isGeneratingFrames) {
        // Map 0.0-1.0 to 0-80%
        const auto percent = qRound(progress * 80.0);
        emit progressUpdated(m_item, percent);
    }
}

void HtmlGeneratorJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    AbstractJob::onFinished(exitCode, exitStatus);

    if (exitCode == 0 && QFileInfo::exists(m_outputPath)) {
        // Automatically open the captured file
        QTimer::singleShot(0, this, [this]() { MAIN.open(m_outputPath); });
    }
}

void HtmlGeneratorJob::onOpenTriggered()
{
    MAIN.open(m_outputPath);
}
