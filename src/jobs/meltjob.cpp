/*
 * Copyright (c) 2012-2026 Meltytech, LLC
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

#include "meltjob.h"

#include "Logger.h"
#include "dialogs/textviewerdialog.h"
#include "mainwindow.h"
#include "settings.h"
#include "util.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QTimer>

MeltJob::MeltJob(const QString &name,
                 const QString &xml,
                 int frameRateNum,
                 int frameRateDen,
                 QThread::Priority priority)
    : AbstractJob(name, priority)
    , m_isStreaming(false)
    , m_previousPercent(0)
    , m_currentFrame(0)
    , m_useMultiConsumer(false)
{
    setTarget(name);
    if (!xml.isEmpty()) {
        QAction *action = new QAction(tr("View XML"), this);
        action->setToolTip(tr("View the MLT XML for this job"));
        connect(action, SIGNAL(triggered()), this, SLOT(onViewXmlTriggered()));
        m_standardActions << action;
        m_xml.reset(Util::writableTemporaryFile(name, "shotcut-XXXXXX.mlt"));
        if (m_xml->open()) {
            m_xml->write(xml.toUtf8());
            m_xml->close();
        }
    } else {
        // Not an EncodeJob
        QAction *action = new QAction(tr("Open"), this);
        action->setData("Open");
        action->setToolTip(tr("Open the output file in the Shotcut player"));
        connect(action, SIGNAL(triggered()), this, SLOT(onOpenTiggered()));
        m_successActions << action;

        action = new QAction(tr("Show In Folder"), this);
        action->setToolTip(tr("Show In Files"));
        connect(action, SIGNAL(triggered()), this, SLOT(onShowInFilesTriggered()));
        m_successActions << action;

        action = new QAction(tr("Show In Folder"), this);
        action->setToolTip(tr("Show In Folder"));
        connect(action, SIGNAL(triggered()), this, SLOT(onShowFolderTriggered()));
        m_successActions << action;
    }
    if (frameRateNum > 0 && frameRateDen > 0)
        m_profile.set_frame_rate(frameRateNum, frameRateDen);
}

void MeltJob::onOpenTiggered()
{
    MAIN.open(objectName().toUtf8().constData());
}

void MeltJob::onShowFolderTriggered()
{
    Util::showInFolder(objectName());
}

void MeltJob::onShowInFilesTriggered()
{
    MAIN.showInFiles(objectName());
}

MeltJob::MeltJob(const QString &name,
                 const QString &xml,
                 const QStringList &args,
                 int frameRateNum,
                 int frameRateDen)
    : MeltJob(name, xml, frameRateNum, frameRateDen)
{
    m_args = args;
}

MeltJob::MeltJob(const QString &name, const QStringList &args, int frameRateNum, int frameRateDen)
    : MeltJob(name, QString(), frameRateNum, frameRateDen)
{
    m_args = args;
}

MeltJob::~MeltJob()
{
    LOG_DEBUG() << "begin";
}

void MeltJob::start()
{
    if (m_args.isEmpty() && !m_xml) {
        AbstractJob::start();
        LOG_ERROR() << "the job XML is empty!";
        appendToLog("Error: the job XML is empty!\n");
        QTimer::singleShot(0, this, [=]() { emit finished(this, false); });
        return;
    }
    QString shotcutPath = qApp->applicationDirPath();
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    QFileInfo meltPath(shotcutPath, "melt-7");
#else
    QFileInfo meltPath(shotcutPath, "melt");
#endif
    setReadChannel(QProcess::StandardError);
    QStringList args;
    args << "-verbose";
    args << "-progress2";
    args << "-abort";
    if (!m_xml.isNull()) {
        if (m_useMultiConsumer) {
            args << "xml:" + QUrl::toPercentEncoding(xmlPath()) + "?multi:1";
        } else {
            args << "xml:" + QUrl::toPercentEncoding(xmlPath());
        }
    }
    if (m_args.size() > 0) {
        args.append(m_args);
    }
    if (m_in > -1) {
        args << QStringLiteral("in=%1").arg(m_in);
    }
    if (m_out > -1) {
        args << QStringLiteral("out=%1").arg(m_out);
    }
    LOG_DEBUG() << meltPath.absoluteFilePath() + " " + args.join(' ');
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
#ifndef Q_OS_MAC
    // These environment variables fix rich text rendering for high DPI
    // fractional or otherwise.
    env.insert("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
    env.insert("QT_SCALE_FACTOR_ROUNDING_POLICY", "PassThrough");
#endif
    if (!Settings.encodeHardwareDecoder()) {
        env.remove("MLT_AVFORMAT_HWACCEL");
    }
    env.remove("MLT_AVFORMAT_HWACCEL_PPS");
    setProcessEnvironment(env);
#ifdef Q_OS_WIN
    if (m_isStreaming)
        args << "-getc";
#endif
    AbstractJob::start(meltPath.absoluteFilePath(), args);
}

QString MeltJob::xml()
{
    if (m_xml->open()) {
        QString s(m_xml->readAll());
        m_xml->close();
        return s;
    } else {
        return QString();
    }
}

void MeltJob::setIsStreaming(bool streaming)
{
    m_isStreaming = streaming;
}

void MeltJob::setUseMultiConsumer(bool multi)
{
    m_useMultiConsumer = multi;
}

void MeltJob::setInAndOut(int in, int out)
{
    m_in = in;
    m_out = out;
}

void MeltJob::onViewXmlTriggered()
{
    TextViewerDialog dialog(&MAIN, true);
    dialog.setWindowTitle(tr("MLT XML"));
    dialog.setText(xml());
    dialog.exec();
}

void MeltJob::onReadyRead()
{
    QString msg;
    do {
        msg = readLine();
        int index = msg.indexOf("Frame:");
        if (index > -1) {
            index += 6;
            int comma = msg.indexOf(',', index);
            m_currentFrame = msg.mid(index, comma - index).toInt();
        }
        index = msg.indexOf("percentage:");
        if (index > -1) {
            int percent = msg.mid(index + 11).toInt();
            if (percent > m_previousPercent) {
                emit progressUpdated(m_item, percent);
                QCoreApplication::processEvents();
                m_previousPercent = percent;
            }
        } else {
            appendToLog(msg);
        }
    } while (!msg.isEmpty());
}

void MeltJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    AbstractJob::onFinished(exitCode, exitStatus);
    if (exitStatus != QProcess::NormalExit && exitCode != 0 && !stopped()) {
        Mlt::Producer producer(m_profile, "colour:");
        QString time = QString::fromLatin1(producer.frames_to_time(m_currentFrame));
        emit finished(this, false, time);
    }
}
