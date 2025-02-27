/*
 * Copyright (c) 2024 Meltytech, LLC
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
                       QThread::Priority priority)
    : AbstractJob(name, priority)
    , m_iWavFile(iWavFile)
    , m_oSrtFile(oSrtFile)
    , m_lang(lang)
    , m_translate(translate)
    , m_maxLength(maxLength)
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
    args << "-f" << m_iWavFile;
    args << "-m" << modelPath;
    args << "-l" << m_lang;
    if (m_translate) {
        args << "-tr";
    }
    args << "-of" << of;
    args << "-osrt";
    args << "-pp";
    args << "-ml" << QString::number(m_maxLength);
    args << "-sow";

#if QT_POINTER_SIZE == 4
    // Limit to 1 rendering thread on 32-bit process to reduce memory usage.
    auto threadCount = 1;
#else
    auto threadCount = qMax(1, QThread::idealThreadCount() - 1);
#endif
    args << "-t" << QString::number(threadCount);

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
