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

#include "screencapturejob.h"

#include "Logger.h"
#include "ffmpegjob.h"
#include "jobqueue.h"
#include "mainwindow.h"
#include "postjobaction.h"
#include "screencapture/screencapture.h"
#include "settings.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimer>
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#endif

ScreenCaptureJob::ScreenCaptureJob(const QString &name,
                                   const QString &filename,
                                   const QRect &captureRect,
                                   bool recordAudio)
    : AbstractJob(name, QThread::NormalPriority)
    , m_filename(filename)
    , m_rect(captureRect)
    , m_isAutoOpen(true)
    , m_recordAudio(recordAudio)
{
    QAction *action = new QAction(tr("Open"), this);
    action->setToolTip(tr("Open the capture"));
    action->setData("Open");
    connect(action, SIGNAL(triggered(bool)), this, SLOT(onOpenTriggered()));
    m_successActions << action;
    m_standardActions.clear();
}

ScreenCaptureJob::~ScreenCaptureJob() {}

void ScreenCaptureJob::start()
{
    LOG_DEBUG() << "starting screen capture job";

    // Create and start progress timer
    connect(&m_progressTimer, &QTimer::timeout, this, [=]() {
        auto secs = time().elapsed() / 1000;
        emit progressUpdated(m_item, -secs);
    });
    m_progressTimer.start(1000); // Update every second

    QStringList args;
#ifdef Q_OS_MAC
    args << "-C";
    args << "-g";
    args << "-k";
    args << "-v";
    args << m_filename;
    LOG_DEBUG() << "screencapture " + args.join(' ');
    AbstractJob::start("screencapture", args);
#else
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // On Linux, check if we're on Wayland and try D-Bus first
    if (ScreenCapture::isWayland()) {
        if (startWaylandRecording()) {
            AbstractJob::start("sleep", {"infinity"});
            return;
        }
        LOG_WARNING() << "Wayland D-Bus recording failed, falling back to ffmpeg (may not work)";
    }
#endif
    QString vcodec("libx264");
    args << "-f"
#ifdef Q_OS_WIN
         << "gdigrab";
    args << "-offset_x" << QString::number(m_rect.x());
    args << "-offset_y" << QString::number(m_rect.y());
    args << "-framerate" << QString::number(MLT.profile().fps());
    args << "-video_size" << QString("%1x%2").arg(m_rect.width()).arg(m_rect.height());
    args << "-i"
         << "desktop";
    if (m_recordAudio) {
        args << "-f"
             << "dshow";
        args << "-i"
             << "audio=" + Settings.audioInput();
    }
    if (Settings.encodeUseHardware() && !Settings.encodeHardware().isEmpty()) {
        vcodec = "h264_mf";
        args << "-rate_control"
             << "quality";
        args << "-q:v"
             << "90";
        args << "-pix_fmt"
             << "nv12";
        args << "-hw_encoding"
             << "true";
#else
         << "x11grab";
    args << "-grab_x" << QString::number(m_rect.x());
    args << "-grab_y" << QString::number(m_rect.y());
    args << "-framerate" << QString::number(MLT.profile().fps());
    args << "-video_size" << QString("%1x%2").arg(m_rect.width()).arg(m_rect.height());
    args << "-i"
         << ":0.0";
    if (m_recordAudio) {
        args << "-f"
             << "pulse";
        args << "-i" << Settings.audioInput();
    }
    if (Settings.encodeUseHardware() && !Settings.encodeHardware().isEmpty()) {
        if (Settings.encodeHardware().contains("h264_nvenc")) {
            vcodec = "h264_nvenc";
            args << "-preset"
                 << "p4";
            args << "-tune"
                 << "hq";
            args << "-rc"
                 << "vbr";
            args << "-cq"
                 << "19";
            args << "-b:v"
                 << "0";
        } else if (Settings.encodeHardware().contains("h264_vaapi")) {
            vcodec = "h264_vaapi";
            args << "-qp"
                 << "18";
            args << "-init_hw_device"
                 << "vaapi=vaapi0:";
            args << "-filter_hw_device"
                 << "vaapi0";
            args << "-vf"
                 << "format=nv12,hwupload";
            args << "-quality"
                 << "1";
            args << "-rc_mode"
                 << "CQP";
        }
#endif
    } else {
        args << "-crf"
             << "18";
        args << "-preset"
             << "veryfast";
        args << "-tune"
             << "film";
        args << "-pix_fmt"
             << "yuv420p";
    }
    args << "-codec:v" << vcodec;
    if (vcodec == "h264_mf") {
        args << "-profile:v"
             << "100";
    } else {
        args << "-profile:v"
             << "high";
    }
    args << "-bf"
         << "2";
    args << "-g" << QString::number(qRound(2.0 * MLT.profile().fps()));
    args << "-color_range"
         << "jpeg";
    args << "-color_primaries"
         << "bt709";
    args << "-color_trc"
         << "bt709";
    args << "-colorspace"
         << "bt709";
    args << "-y" << m_filename;
    QString shotcutPath = qApp->applicationDirPath();
    QFileInfo ffmpegPath(shotcutPath, "ffmpeg");
    setReadChannel(QProcess::StandardError);
    LOG_DEBUG() << ffmpegPath.absoluteFilePath() + " " + args.join(' ');
    AbstractJob::start(ffmpegPath.absoluteFilePath(), args);
#endif
}

void ScreenCaptureJob::stop()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if (m_dbusService == DBusService::GNOME) {
        // Stop GNOME screencast
        auto bus = QDBusConnection::sessionBus();
        QDBusMessage msg = QDBusMessage::createMethodCall("org.gnome.Shell.Screencast",
                                                          "/org/gnome/Shell/Screencast",
                                                          "org.gnome.Shell.Screencast",
                                                          "StopScreencast");
        QDBusPendingCall async = bus.asyncCall(msg);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);
        connect(watcher,
                &QDBusPendingCallWatcher::finished,
                this,
                [this](QDBusPendingCallWatcher *w) {
                    QDBusPendingReply<bool> reply = *w;
                    LOG_DEBUG() << "GNOME Screencast stopped";
                    // emit finished(this, true);
                    w->deleteLater();
                });
    } else if (m_dbusService == DBusService::KDE) {
        // For KDE Spectacle, user stops recording via the Spectacle UI
        // We just wait for the signals
        LOG_DEBUG() << "Waiting for KDE Spectacle to finish recording (user controlled)";
    }
    if (m_dbusService != DBusService::None) {
        AbstractJob::stop();
        setKilled(false);
        return;
    }
#endif
    // Try to terminate gracefully
    write("q");
    QTimer::singleShot(1000, this, [this]() {
        if (m_progressTimer.isActive())
            AbstractJob::stop();
    });
}

void ScreenCaptureJob::onOpenTriggered()
{
    MAIN.open(m_filename);
}

void ScreenCaptureJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_progressTimer.stop();

    LOG_DEBUG() << "screen capture job finished with exit code" << exitCode;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if (m_dbusService == DBusService::GNOME) {
        LOG_INFO() << "job succeeeded";
        appendToLog(QStringLiteral("Completed successfully in %1\n")
                        .arg(QTime::fromMSecsSinceStartOfDay(time().elapsed()).toString()));
        emit progressUpdated(m_item, 100);
        emit finished(this, true);

        if (!m_actualFilename.endsWith(".mp4")) {
            // Remux the file from Matroska to chosen WebM
            QFileInfo fileInfo(m_filename);
            QString inputFileName = m_actualFilename.isEmpty()
                                        ? fileInfo.path() + "/" + fileInfo.completeBaseName()
                                              + ".mkv"
                                        : m_actualFilename;
            if (m_isAutoOpen && QFileInfo::exists(inputFileName)) {
                m_isAutoOpen = false;

                // Create FFmpeg remux job
                QStringList args;
                args << "-i" << inputFileName;
                args << "-c"
                     << "copy";
                args << "-y" << m_filename;

                FfmpegJob *remuxJob = new FfmpegJob(m_filename, args, false);
                remuxJob->setLabel(tr("Remux %1").arg(fileInfo.fileName()));
                remuxJob->setPostJobAction(
                    new OpenPostJobAction(inputFileName, m_filename, inputFileName));
                JOBS.add(remuxJob);

                return;
            }
        }
        if (!m_actualFilename.isEmpty()) {
            m_filename = m_actualFilename;
        }
    }
    if (m_dbusService != DBusService::None) {
        exitCode = 0; // ignore exit code from sleep
        exitStatus = QProcess::NormalExit;
    }
#endif
    AbstractJob::onFinished(exitCode, exitStatus);

    if (m_isAutoOpen && exitCode == 0 && QFileInfo::exists(m_filename)) {
        // Automatically open the captured file
        m_isAutoOpen = false;
        QTimer::singleShot(0, this, [this]() { MAIN.open(m_filename); });
    }
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
bool ScreenCaptureJob::startWaylandRecording()
{
    // Check desktop environment
    QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower();
    LOG_DEBUG() << "XDG_CURRENT_DESKTOP:" << desktop;

    // Try GNOME first
    if (desktop.contains("gnome")) {
        if (startGnomeScreencast()) {
            return true;
        }
    }

    // Try KDE
    if (desktop.contains("kde") || desktop.contains("plasma")) {
        if (startKdeSpectacle()) {
            return true;
        }
    }

    // If we reach here, neither GNOME nor KDE worked
    // This shouldn't happen as MainWindow should have launched OBS Studio
    LOG_ERROR() << "Desktop environment not GNOME or KDE, and fallback not available in job";
    return false;
}

bool ScreenCaptureJob::startGnomeScreencast()
{
    LOG_DEBUG() << "Attempting GNOME Shell Screencast";

    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        LOG_WARNING() << "DBus session bus not connected";
        return false;
    }

    // Check if the service is available
    QDBusMessage checkMsg = QDBusMessage::createMethodCall("org.gnome.Shell.Screencast",
                                                           "/org/gnome/Shell/Screencast",
                                                           "org.freedesktop.DBus.Introspectable",
                                                           "Introspect");
    QDBusPendingCall async = bus.asyncCall(checkMsg, 1000);
    async.waitForFinished();
    QDBusPendingReply<QString> reply = async;
    if (reply.isError()) {
        LOG_WARNING() << "GNOME Shell Screencast service not available:" << reply.error().message();
        return false;
    }

    // Prepare options
    QVariantMap options;
    options.insert("framerate", static_cast<int>(MLT.profile().fps()));
    options.insert("draw-cursor", true);

    // Call ScreencastArea
    LOG_DEBUG() << "Recording region:" << m_rect;
    auto msg = QDBusMessage::createMethodCall("org.gnome.Shell.Screencast",
                                              "/org/gnome/Shell/Screencast",
                                              "org.gnome.Shell.Screencast",
                                              "ScreencastArea");
    msg << m_rect.x();
    msg << m_rect.y();
    msg << m_rect.width();
    msg << m_rect.height();
    QFileInfo fileInfo(m_filename);
    msg << fileInfo.path() + "/" + fileInfo.completeBaseName() + ".mkv";
    msg << options;
    QDBusPendingCall screencastCall = bus.asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(screencastCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<bool, QString> reply = *w;
        if (reply.isError()) {
            LOG_ERROR() << "GNOME Screencast call failed:" << reply.error().message();
            AbstractJob::stop();
        } else {
            bool success = reply.argumentAt<0>();
            QString filename = reply.argumentAt<1>();
            LOG_DEBUG() << "GNOME Screencast started, success:" << success
                        << "filename:" << filename;
            m_actualFilename = filename;
        }
        w->deleteLater();
    });

    m_dbusService = DBusService::GNOME;
    LOG_INFO() << "Started GNOME Shell Screencast to:"
               << fileInfo.path() + "/" + fileInfo.completeBaseName() + ".mkv";

    // Job will be controlled via stop() method
    return true;
}

bool ScreenCaptureJob::startKdeSpectacle()
{
    LOG_DEBUG() << "Attempting KDE Spectacle recording";

    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        LOG_WARNING() << "DBus session bus not connected";
        return false;
    }

    // Connect to RecordingTaken and RecordingFailed signals
    bool connected = bus.connect("org.kde.Spectacle",
                                 "/",
                                 "org.kde.Spectacle",
                                 "RecordingTaken",
                                 this,
                                 SLOT(onDBusRecordingTaken(QString)));
    if (!connected) {
        LOG_WARNING() << "Failed to connect to RecordingTaken signal";
        return false;
    }

    connected = bus.connect("org.kde.Spectacle",
                            "/",
                            "org.kde.Spectacle",
                            "RecordingFailed",
                            this,
                            SLOT(onDBusRecordingFailed()));
    if (!connected) {
        LOG_WARNING() << "Failed to connect to RecordingFailed signal";
        bus.disconnect("org.kde.Spectacle",
                       "/",
                       "org.kde.Spectacle",
                       "RecordingTaken",
                       this,
                       SLOT(onDBusRecordingTaken(QString)));
        return false;
    }

    // Call RecordRegion for recording
    LOG_DEBUG() << "Recording region:" << m_rect;
    auto msg = QDBusMessage::createMethodCall("org.kde.Spectacle",
                                              "/",
                                              "org.kde.Spectacle",
                                              m_rect.isNull() ? "RecordScreen" : "RecordRegion");
    msg << 1; // includeMousePointer
    QDBusPendingCall async = bus.asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<> reply = *w;
        if (reply.isError()) {
            LOG_ERROR() << "KDE Spectacle recording call failed:" << reply.error().message();
            if (m_rect.isNull()) {
                AbstractJob::stop();
            } else {
                m_rect = QRect();
                w->deleteLater();
                startKdeSpectacle();
            }
        } else {
            LOG_DEBUG() << "KDE Spectacle recording initiated";
        }
        w->deleteLater();
    });

    m_dbusService = DBusService::KDE;
    LOG_INFO() << "Started KDE Spectacle screen recording";

    return true;
}

void ScreenCaptureJob::onDBusRecordingTaken(const QString &fileName)
{
    LOG_DEBUG() << "Recording taken, file:" << fileName;

    // Move/copy the file to our desired location if different
    if (!fileName.isEmpty() && fileName != m_filename) {
        QFile source(fileName);
        if (QFile::exists(m_filename)) {
            QFile::remove(m_filename);
        }
        if (source.copy(m_filename)) {
            LOG_INFO() << "Copied recording from" << fileName << "to" << m_filename;
            source.remove(); // Remove the original
        } else {
            LOG_WARNING() << "Failed to copy recording, using original location";
            m_filename = fileName;
        }
    }

    auto bus = QDBusConnection::sessionBus();
    bus.disconnect("org.kde.Spectacle",
                   "/",
                   "org.kde.Spectacle",
                   "RecordingTaken",
                   this,
                   SLOT(onDBusRecordingTaken(QString)));
    bus.disconnect("org.kde.Spectacle",
                   "/",
                   "org.kde.Spectacle",
                   "RecordingFailed",
                   this,
                   SLOT(onDBusRecordingFailed()));

    stop();
}

void ScreenCaptureJob::onDBusRecordingFailed()
{
    LOG_ERROR() << "Recording failed";

    auto bus = QDBusConnection::sessionBus();
    bus.disconnect("org.kde.Spectacle",
                   "/",
                   "org.kde.Spectacle",
                   "RecordingTaken",
                   this,
                   SLOT(onDBusRecordingTaken(QString)));
    bus.disconnect("org.kde.Spectacle",
                   "/",
                   "org.kde.Spectacle",
                   "RecordingFailed",
                   this,
                   SLOT(onDBusRecordingFailed()));

    AbstractJob::stop();
}
#endif
