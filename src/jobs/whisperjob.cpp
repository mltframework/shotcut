/*
 * Copyright (c) 2024-2026 Meltytech, LLC
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

#include "whisperjob.h"

#include "Logger.h"
#include "dialogs/textviewerdialog.h"
#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QTimer>

WhisperJob::WhisperJob(const QString &name,
                       const QString &iWavFile,
                       const QString &oSrtFile,
                       const QString &lang,
                       bool translate,
                       int maxLength,
                       bool useGpu,
                       QThread::Priority priority)
    : AbstractJob(name, priority)
    , m_iWavFile(iWavFile)
    , m_oSrtFile(oSrtFile)
    , m_lang(lang)
    , m_translate(translate)
    , m_maxLength(maxLength)
    , m_useGpu(useGpu)
    , m_retryingWithoutGpu(false)
    , m_previousPercent(0)
{
    setTarget(oSrtFile);
}

WhisperJob::~WhisperJob()
{
    LOG_DEBUG() << "begin";
}

void WhisperJob::start()
{
    QString whisperPath = Settings.whisperExe();
    auto modelPath = Settings.whisperModel();

    setReadChannel(QProcess::StandardOutput);
    setProcessChannelMode(QProcess::MergedChannels);
    QString of = m_oSrtFile;
    of.remove(".srt");
    QStringList args;
    args << "--file" << m_iWavFile;
    args << "--model" << modelPath;
    args << "--language" << m_lang;
    if (m_translate) {
        args << "--translate";
    }
    args << "--output-file" << of;
    args << "--output-srt";
    args << "--print-progress";
    args << "--max-len" << QString::number(m_maxLength);
    args << "--split-on-word";

#if QT_POINTER_SIZE == 4
    // Limit to 1 rendering thread on 32-bit process to reduce memory usage.
    auto threadCount = 1;
#else
    auto threadCount = qMax(1, QThread::idealThreadCount() - 1);
#endif
    args << "--threads" << QString::number(threadCount);

    if (!m_useGpu || m_retryingWithoutGpu) {
        args << "--no-gpu";
    }

    LOG_DEBUG() << whisperPath + " " + args.join(' ');
    AbstractJob::start(whisperPath, args);
    emit progressUpdated(m_item, 0);
}

void WhisperJob::onViewSrtTriggered()
{
    QFile srtFile(m_oSrtFile);
    QString text = srtFile.readAll();

    TextViewerDialog dialog(&MAIN, true);
    dialog.setWindowTitle(tr("SRT"));
    dialog.setText(text);
    dialog.exec();
}

void WhisperJob::onReadyRead()
{
    QString msg;
    do {
        msg = readLine();
        if (!msg.isEmpty()) {
            int index = msg.indexOf("progress = ");
            if (index > -1) {
                QString num = msg.mid(index + 11).remove("%").trimmed();
                int percent = num.toInt();
                if (percent != m_previousPercent) {
                    emit progressUpdated(m_item, percent);
                    QCoreApplication::processEvents();
                    m_previousPercent = percent;
                }
            }
            appendToLog(msg);
        }
    } while (!msg.isEmpty());
}

void WhisperJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if ((exitStatus != QProcess::NormalExit || exitCode != 0) && !stopped() && m_useGpu
        && !m_retryingWithoutGpu) {
        m_retryingWithoutGpu = true;
        m_previousPercent = 0;
        QString message(tr("Speech to Text job failed; trying again without GPU."));
        MAIN.showStatusMessage(message);
        appendToLog(message.append("\n"));
        QTimer::singleShot(0, this, &WhisperJob::start);
        return;
    }
    if (m_retryingWithoutGpu && exitStatus == QProcess::NormalExit && exitCode == 0) {
        Settings.setWhisperUseGpu(false);
    }
    AbstractJob::onFinished(exitCode, exitStatus);
}
