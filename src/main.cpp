/*
 * Copyright (c) 2011-2026 Meltytech, LLC
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

#include "ConsoleAppender.h"
#include "FileAppender.h"
#include "Logger.h"
#include "gpuinfo.h"
#include "mainwindow.h"
#include "settings.h"

#include <framework/mlt_log.h>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSysInfo>
#include <QTimer>
#include <QtGlobal>
#include <QtWidgets>

#ifdef Q_OS_MAC
#include "macos.h"
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#if defined(QT_DEBUG) && !defined(__ARM_ARCH)
#include <exchndl.h>
#endif
extern "C" {
// Inform the driver we could make use of the discrete gpu
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#endif

static constexpr int kMaxCacheCount = 5000;
constexpr const auto kWatchdogTimeoutMs = 30000;
constexpr const auto kWatchdogEnvVar = "SHOTCUT_WATCHDOG";
// How long the watchdog parent waits for kStartupReadyMarker before assuming the child is hung.
constexpr const auto kStartupHangTimeoutMs = 240000;
// Printed by the child to stdout once its main window is shown; watched for by the parent.
constexpr const auto kStartupReadyMarker = "[Info   ] Shotcut window shown";

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
static QString s_qpaPlatformNote;

// Read the stored drawing method before QApplication exists (the QPA platform
// plugin must be chosen before that). This duplicates the settings file lookup
// done by ShotcutSettings because that singleton requires QCoreApplication.
static bool isVulkanDrawMethod(int argc, char **argv, QString *detail)
{
    if (::qgetenv("QSG_RHI_BACKEND").toLower() == QByteArrayLiteral("vulkan")) {
        *detail = QStringLiteral("QSG_RHI_BACKEND=vulkan");
        return true;
    }
    QString appDataDir;
    for (int i = 1; i < argc; i++) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg.startsWith(QStringLiteral("--appdata="))) {
            appDataDir = arg.mid(10);
            break;
        } else if (arg == QStringLiteral("--appdata") && i + 1 < argc) {
            appDataDir = QString::fromLocal8Bit(argv[i + 1]);
            break;
        }
    }
    if (appDataDir.isEmpty()) {
        QSettings settings(QSettings::NativeFormat,
                           QSettings::UserScope,
                           QStringLiteral("Meltytech"),
                           QStringLiteral("Shotcut"));
        appDataDir = settings.value(QStringLiteral("appdatadir")).toString();
        if (appDataDir.isEmpty() || !QFile::exists(appDataDir + QStringLiteral("/shotcut.ini"))) {
            const int value
                = settings.value(QStringLiteral("opengl"), Qt::AA_UseDesktopOpenGL).toInt();
            *detail = QStringLiteral("opengl=%1 from %2").arg(value).arg(settings.fileName());
            return value == QSGRendererInterface::Vulkan;
        }
    }
    QSettings settings(appDataDir + QStringLiteral("/shotcut.ini"), QSettings::IniFormat);
    const int value = settings.value(QStringLiteral("opengl"), Qt::AA_UseDesktopOpenGL).toInt();
    *detail = QStringLiteral("opengl=%1 from %2").arg(value).arg(settings.fileName());
    return value == QSGRendererInterface::Vulkan;
}
#endif

static void mlt_log_handler(void *service, int mlt_level, const char *format, va_list args)
{
    if (mlt_level > mlt_log_get_level())
        return;

    enum Logger::LogLevel cuteLoggerLevel = Logger::Fatal;
    switch (mlt_level) {
    case MLT_LOG_DEBUG:
        cuteLoggerLevel = Logger::Trace;
        break;
    case MLT_LOG_ERROR:
    case MLT_LOG_FATAL:
    case MLT_LOG_PANIC:
        cuteLoggerLevel = Logger::Error;
        break;
    case MLT_LOG_INFO:
        cuteLoggerLevel = Logger::Info;
        break;
    case MLT_LOG_VERBOSE:
        cuteLoggerLevel = Logger::Debug;
        break;
    case MLT_LOG_WARNING:
        cuteLoggerLevel = Logger::Warning;
        break;
    }
    QString message;
    mlt_properties properties = service ? MLT_SERVICE_PROPERTIES((mlt_service) service) : NULL;
    if (properties) {
        char *mlt_type = mlt_properties_get(properties, "mlt_type");
        char *service_name = mlt_properties_get(properties, "mlt_service");
        char *resource = mlt_properties_get(properties, "resource");
        if (!resource || resource[0] != '<' || resource[strlen(resource) - 1] != '>')
            mlt_type = mlt_properties_get(properties, "mlt_type");
        if (service_name)
            message = QStringLiteral("[%1 %2] ").arg(mlt_type, service_name);
        else
            message = QString::asprintf("[%s %p] ", mlt_type, service);
        if (resource)
            message.append(QStringLiteral("\"%1\" ").arg(resource));
        message.append(QString::vasprintf(format, args));
        message.replace('\n', "");
    } else {
        message = QString::vasprintf(format, args);
        message.replace('\n', "");
    }
    cuteLogger->write(cuteLoggerLevel,
                      __FILE__,
                      __LINE__,
                      "MLT",
                      cuteLogger->defaultCategory().toLatin1().constData(),
                      message);
}

class Application : public QApplication
{
public:
    MainWindow *mainWindow{nullptr};
    QTranslator qtTranslator;
    QTranslator qtBaseTranslator;
    QTranslator shotcutTranslator;
    QStringList resourceArg;
    QStringList fileOpenEventArgs;
    bool isFullScreen;
    QString appDirArg;

    Application(int &argc, char **argv)
        : QApplication(argc, argv)
    {
        auto appPath = applicationDirPath();
        QDir dir(appPath);

#ifdef Q_OS_WIN
#include <winbase.h>
        SetDllDirectoryA(appPath.toLocal8Bit());
        CreateMutexA(NULL, FALSE, "Meltytech Shotcut Running Mutex");
#else
        dir.cdUp();
#endif
#ifdef Q_OS_MAC
        dir.cd("PlugIns");
        dir.cd("qt");
#else
        dir.cd("lib");
        dir.cd("qt6");
#endif
        addLibraryPath(dir.absolutePath());
        setOrganizationName("Meltytech");
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        setOrganizationDomain("shotcut.org");
        setDesktopFileName("org.shotcut.Shotcut");
#else
        setOrganizationDomain("meltytech.com");
#endif
        setApplicationName("Shotcut");
        setApplicationVersion(SHOTCUT_VERSION);

        // Process command line options.
        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addVersionOption();
#ifndef Q_OS_WIN
        QCommandLineOption fullscreenOption(
            "fullscreen",
            QCoreApplication::translate("main", "Fill the screen with the Shotcut window."));
        parser.addOption(fullscreenOption);
#endif
        QCommandLineOption noupgradeOption(
            "noupgrade", QCoreApplication::translate("main", "Hide upgrade prompt and menu item."));
        parser.addOption(noupgradeOption);
        QCommandLineOption
            glaxnimateOption("glaxnimate",
                             QCoreApplication::translate("main",
                                                         "Run Glaxnimate instead of Shotcut."));
        parser.addOption(glaxnimateOption);
        QCommandLineOption gpuOption("gpu",
                                     QCoreApplication::translate("main", "Use GPU processing."));
        parser.addOption(gpuOption);
        QCommandLineOption experimentalOption(
            "experimental",
            QCoreApplication::translate("main",
                                        "Enable experimental features (add-on filters menu)."));
        parser.addOption(experimentalOption);
        QCommandLineOption clearRecentOption("clear-recent",
                                             QCoreApplication::translate("main",
                                                                         "Clear Recent on Exit"));
        parser.addOption(clearRecentOption);
        QCommandLineOption appDataOption(
            "appdata",
            QCoreApplication::translate("main", "The directory for app configuration and data."),
            QCoreApplication::translate("main", "directory"));
        parser.addOption(appDataOption);
        QCommandLineOption
            scaleOption("QT_SCALE_FACTOR",
                        QCoreApplication::translate("main",
                                                    "The scale factor for a high-DPI screen"),
                        QCoreApplication::translate("main", "number"));
        parser.addOption(scaleOption);
        scaleOption
            = QCommandLineOption("QT_SCREEN_SCALE_FACTORS",
                                 QCoreApplication::translate(
                                     "main",
                                     "A semicolon-separated list of scale factors for each screen"),
                                 QCoreApplication::translate("main", "list"));
        parser.addOption(scaleOption);
        QCommandLineOption scalePolicyOption(
            "QT_SCALE_FACTOR_ROUNDING_POLICY",
            QCoreApplication::translate("main", "How to handle a fractional display scale: %1")
                .arg("Round, Ceil, Floor, RoundPreferFloor, PassThrough"),
            QCoreApplication::translate("main", "string"),
            "PassThrough");
        parser.addOption(scalePolicyOption);
#if defined(Q_OS_WIN)
        QCommandLineOption sdlAudioDriverOption(
            "SDL_AUDIODRIVER",
            QCoreApplication::translate("main", "Which operating system audio API to use: %1")
                .arg("directsound, wasapi, winmm"),
            QCoreApplication::translate("main", "string"),
            "wasapi");
        parser.addOption(sdlAudioDriverOption);
#elif defined(Q_OS_LINUX)
        QCommandLineOption sdlAudioDriverOption(
            "SDL_AUDIODRIVER",
            QCoreApplication::translate("main", "Which operating system audio API to use: %1")
                .arg("alsa, arts, dsp, esd, jack, pipewire, pulseaudio"),
            QCoreApplication::translate("main", "string"),
            "pulseaudio");
        parser.addOption(sdlAudioDriverOption);
#endif
        parser.addPositionalArgument(
            "[FILE]...",
            QCoreApplication::translate("main", "Zero or more files or folders to open"));
        parser.process(arguments());
        if (parser.isSet(glaxnimateOption)) {
            QStringList args = arguments();
            if (!args.isEmpty())
                args.removeFirst();
            args.removeAll("--glaxnimate");
            QProcess child;
            if (child.startDetached(Settings.glaxnimatePath(), args))
                ::exit(EXIT_SUCCESS);
        }

#ifdef Q_OS_WIN
        isFullScreen = false;
#else
        isFullScreen = parser.isSet(fullscreenOption);
#endif
        setProperty("noupgrade", parser.isSet(noupgradeOption));
        setProperty("clearRecent", parser.isSet(clearRecentOption));
#ifdef QT_DEBUG
        setProperty("experimental", true);
#else
        setProperty("experimental", parser.isSet(experimentalOption));
#endif
        if (!parser.value(appDataOption).isEmpty()) {
            appDirArg = parser.value(appDataOption);
            ShotcutSettings::setAppDataForSession(appDirArg);
        }
        if (parser.isSet(gpuOption))
            Settings.setProcessingMode(ShotcutSettings::Linear10GpuCpu);
        if (!parser.positionalArguments().isEmpty())
            resourceArg = parser.positionalArguments();

        // Startup logging.
        dir.setPath(Settings.appDataLocation());
        if (!dir.exists())
            dir.mkpath(dir.path());
        const auto logFileName = dir.filePath("shotcut-log.txt");
        if (QFile::exists(logFileName)) {
            const auto previousLogName = dir.filePath("shotcut-log.bak");
            if (QFile::exists(previousLogName))
                QFile::remove(previousLogName);
            if (!QFile::rename(logFileName, previousLogName))
                LOG_WARNING() << "Failed to rename backup log file" << previousLogName;
        }
        FileAppender *fileAppender = new FileAppender(logFileName);
        fileAppender->setFormat("[%{type:-7}] <%{function}> %{message}\n");
        cuteLogger->registerAppender(fileAppender);
#ifndef NDEBUG
        // Only log to console in dev debug builds.
        ConsoleAppender *consoleAppender = new ConsoleAppender();
        consoleAppender->setFormat(fileAppender->format());
        cuteLogger->registerAppender(consoleAppender);

        mlt_log_set_level(MLT_LOG_VERBOSE);
#else
        mlt_log_set_level(MLT_LOG_INFO);
#endif
        mlt_log_set_callback(mlt_log_handler);
        cuteLogger->logToGlobalInstance("qml", true);

#if defined(Q_OS_WIN)
        dir.setPath(appPath);
#endif
        // Load translations
        QString locale = Settings.language();
        dir.setPath(appPath);
#if defined(Q_OS_MAC)
        dir.cdUp();
        dir.cd("Resources");
        dir.cd("shotcut");
        dir.cd("translations");
#elif defined(Q_OS_WIN)
        dir.cd("share");
        dir.cd("translations");
#else
        dir.cdUp();
        dir.cd("share");
        dir.cd("shotcut");
        dir.cd("translations");
#endif
        if (locale.startsWith("pt_"))
            locale = "pt";
        else if (locale.startsWith("en_"))
            locale = "en";
        if (qtTranslator.load("qt_" + locale, QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
            installTranslator(&qtTranslator);
        else if (qtTranslator.load("qt_" + locale, dir.absolutePath()))
            installTranslator(&qtTranslator);
        if (qtBaseTranslator.load("qtbase_" + locale,
                                  QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
            installTranslator(&qtBaseTranslator);
        else if (qtBaseTranslator.load("qtbase_" + locale, dir.absolutePath()))
            installTranslator(&qtBaseTranslator);
        if (shotcutTranslator.load("shotcut_" + Settings.language(), dir.absolutePath()))
            installTranslator(&shotcutTranslator);
    }

    ~Application()
    {
        delete mainWindow;
        LOG_DEBUG() << "exiting";
    }

protected:
    bool event(QEvent *event)
    {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
            if (mainWindow) {
                mainWindow->openMultiple(QStringList() << openEvent->file());
            } else {
                // Running as the watchdog parent — forward to the child via IPC.
                // The child may need several seconds to start its IPC server,
                // so retry the connection with 1-second intervals.
                bool sent = false;
                for (int i = 0; i < 15 && !sent; ++i) {
                    QLocalSocket socket;
                    socket.connectToServer("shotcut-file-open");
                    if (socket.waitForConnected(1000)) {
                        socket.write(openEvent->file().toUtf8());
                        socket.waitForBytesWritten(500);
                        socket.disconnectFromServer();
                        sent = true;
                    }
                }
                if (!sent)
                    fileOpenEventArgs << openEvent->file();
            }
            return true;
        }
        return QApplication::event(event);
    }
};

int main(int argc, char **argv)
{
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
    // Attach to a parent console for console logging.
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE *dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
    }
#if !defined(__ARM_ARCH)
    ExcHndlInit();
#endif
#endif
#ifndef QT_DEBUG
    ::qputenv("QT_LOGGING_RULES", "*.warning=false");
#endif
    for (int i = 1; i + 1 < argc; i++) {
        if (!::qstrcmp("--QT_SCALE_FACTOR", argv[i])
            || !::qstrcmp("--QT_SCREEN_SCALE_FACTORS", argv[i])) {
            QByteArray value(argv[i + 1]);
            ::qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");
            ::qputenv(value.contains(';') ? "QT_SCREEN_SCALE_FACTORS" : "QT_SCALE_FACTOR", value);
            break;
        }
    }
    if (!::qEnvironmentVariableIsSet("QT_SCALE_FACTOR_ROUNDING_POLICY")) {
        for (int i = 1; i + 1 < argc; i++) {
            if (!::qstrcmp("--QT_SCALE_FACTOR_ROUNDING_POLICY", argv[i])) {
                ::qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY", argv[i + 1]);
                break;
            }
        }
    }
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    for (int i = 1; i + 1 < argc; i++) {
        if (!::qstrcmp("--SDL_AUDIODRIVER", argv[i])) {
            ::qputenv("SDL_AUDIODRIVER", argv[i + 1]);
            break;
        }
    }
#endif

    // The ffmpeg backend (default as of Qt 6.5) is likely using a different version of FFmpeg.
    if (!qEnvironmentVariableIsSet("QT_MEDIA_BACKEND"))
#if defined(Q_OS_MAC)
        qputenv("QT_MEDIA_BACKEND", "darwin");
#elif defined(Q_OS_WIN)
        qputenv("QT_MEDIA_BACKEND", "windows");
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "windows:altgr");
#elif defined(BUILD_MINIMAL_MEDIA_BACKEND)
        qputenv("QT_MEDIA_BACKEND", "minimal");
#else
        ;
#endif

#ifdef Q_OS_MAC
    // Launcher and Spotlight on macOS are not setting this environment
    // variable needed by setlocale() as used by MLT.
    if (QProcessEnvironment::systemEnvironment().value(MLT_LC_NAME).isEmpty()) {
        qputenv(MLT_LC_NAME, QLocale().name().toUtf8());

        QLocale localeByName(
            QLocale(QLocale().language(), QLocale().script(), QLocale().territory()));
        if (QLocale().decimalPoint() != localeByName.decimalPoint()) {
            // If region's numeric format does not match the language's, then we run
            // into problems because we told MLT and libc to use a different numeric
            // locale than actually in use by Qt because it is unable to give numeric
            // locale as a set of ISO-639 codes.
            QLocale::setDefault(localeByName);
            qputenv("LANG", QLocale().name().toUtf8());
        }
    }
    removeMacosTabBar();
    QCoreApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // QtWayland does not draw its client-side window decorations (title bar,
    // resize borders) on a Vulkan surface, so a Wayland session using the Vulkan
    // drawing method gets an undecorated window. Use XWayland instead, where the
    // decorations are drawn by the X11 window manager and Vulkan still works.
    // This must happen before QApplication is constructed.
    {
        const QByteArray platformEnv = ::qgetenv("QT_QPA_PLATFORM").toLower();
        // QT_QPA_PLATFORM may be a ';'-separated preference list, e.g. "wayland;xcb".
        const bool isWayland
            = platformEnv.isEmpty()
                  ? (::qgetenv("XDG_SESSION_TYPE").toLower() == QByteArrayLiteral("wayland")
                     || ::qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
                  : platformEnv.split(';').constFirst().startsWith(QByteArrayLiteral("wayland"));
        const bool hasX11 = ::qEnvironmentVariableIsSet("DISPLAY");
        QString drawMethodDetail;
        const bool isVulkan = isVulkanDrawMethod(argc, argv, &drawMethodDetail);
        s_qpaPlatformNote = QStringLiteral("QT_QPA_PLATFORM=\"%1\" wayland=%2 x11=%3 %4")
                                .arg(QString::fromLocal8Bit(platformEnv))
                                .arg(isWayland)
                                .arg(hasX11)
                                .arg(drawMethodDetail);
        if (isWayland && isVulkan && hasX11) {
            ::qputenv("QT_QPA_PLATFORM", "xcb");
            s_qpaPlatformNote.append(
                QStringLiteral(" - using XWayland because Wayland has no window decorations "
                               "with Vulkan"));
        }
    }
#endif

    Application a(argc, argv);
    int result = EXIT_SUCCESS;
#ifdef Q_OS_WIN
    // Direct the user-selected physical GPU to be used by the Qt RHI (Direct3D)
    // preview/UI via QT_D3D_ADAPTER_INDEX and by the FFmpeg hardware decoder via
    // MLT_AVFORMAT_HWACCEL_DEVICE. These must be set before the RHI is initialized (the
    // adapter index is read lazily when the first scene-graph window is created). In
    // release builds the watchdog child process, and the melt export subprocess, inherit
    // this environment. Values already set by the user take precedence. The adapter index
    // is resolved live from the GPU's stable vendor+device identity because some drivers
    // enumerate adapters in an unstable order across runs. See gpuinfo.cpp and Settings.
    {
        const uint gpuVendorId = Settings.gpuAdapterVendorId();
        if (gpuVendorId != 0) {
            const int gpuAdapterIndex = gpuAdapterIndexFor(gpuVendorId,
                                                           Settings.gpuAdapterDeviceId());
            if (gpuAdapterIndex >= 0) {
                const QByteArray index = QByteArray::number(gpuAdapterIndex);
                if (!qEnvironmentVariableIsSet("QT_D3D_ADAPTER_INDEX"))
                    ::qputenv("QT_D3D_ADAPTER_INDEX", index);
                if (!qEnvironmentVariableIsSet("MLT_AVFORMAT_HWACCEL_DEVICE"))
                    ::qputenv("MLT_AVFORMAT_HWACCEL_DEVICE", index);
            }
        }
    }
#endif
#ifdef QT_DEBUG
    ::qputenv(kWatchdogEnvVar, "1");
#endif
    if (::qEnvironmentVariableIsSet(kWatchdogEnvVar)) {
        QSplashScreen splash(QPixmap(":/icons/shotcut-logo-320x320.png"));

        // Log some basic info.
        LOG_INFO() << "Starting Shotcut version" << SHOTCUT_VERSION;
#if defined(Q_OS_WIN)
        LOG_INFO() << "Windows version" << QSysInfo::productVersion();
#elif defined(Q_OS_MAC)
        LOG_INFO() << "macOS version" << QSysInfo::productVersion();
#else
        if (Settings.playerOldVideoOutput()) {
            QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
            QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
            QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
        } else if (Settings.drawMethod() == QSGRendererInterface::Vulkan) {
            QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);
        } else if (::qgetenv("QSG_RHI_BACKEND").toLower() != QByteArrayLiteral("vulkan")) {
            QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
        }
        LOG_INFO() << "Linux version" << QSysInfo::productVersion();
        LOG_INFO() << "QPA platform =" << a.platformName() << "-" << s_qpaPlatformNote;
#endif
        LOG_INFO() << "number of logical cores =" << QThread::idealThreadCount();
        LOG_INFO() << "locale =" << QLocale();
        LOG_INFO() << "install dir =" << a.applicationDirPath();
        switch (QQuickWindow::graphicsApi()) {
        case QSGRendererInterface::Direct3D11:
            LOG_INFO() << "graphics backend = Direct3D 11";
            break;
        case QSGRendererInterface::Direct3D12:
            LOG_INFO() << "graphics backend = Direct3D 12";
            break;
        case QSGRendererInterface::Metal:
            LOG_INFO() << "graphics backend = Metal";
            break;
        case QSGRendererInterface::OpenGL:
            LOG_INFO() << "graphics backend = OpenGL";
            break;
        case QSGRendererInterface::Vulkan:
            LOG_INFO() << "graphics backend = Vulkan";
            break;
        default:
            LOG_INFO() << "graphics backend = " << QQuickWindow::graphicsApi();
            break;
        }
        Settings.log();

        splash.show();
        a.processEvents();

        // Expire old items from the qmlcache
        auto dir = QDir(
            QStandardPaths::standardLocations(QStandardPaths::CacheLocation).constFirst());
        if (dir.exists() && dir.cd("qmlcache")) {
            auto ls = dir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::Time);
            if (qMax(0, ls.size() - kMaxCacheCount) > 0) {
                LOG_INFO() << "removing" << qMax(0, ls.size() - kMaxCacheCount) << "from"
                           << dir.path();
            }
            for (int i = kMaxCacheCount; i < ls.size(); i++) {
                QString filePath = dir.filePath(ls[i]);
                if (!QFile::remove(filePath)) {
                    LOG_WARNING() << "failed to delete" << filePath;
                }
                if (i % 1000 == 0) {
                    a.processEvents();
                }
            }
        }

        a.setProperty("system-style", a.style()->objectName());
        MainWindow::changeTheme(Settings.theme());
        QQuickStyle::setStyle("Fusion");

        a.mainWindow = &MAIN;
        // Tell the watchdog parent process (if any) that startup got far enough to show a window.
        QObject::connect(a.mainWindow, &MainWindow::windowShown, []() {
            ::fprintf(stdout, "%s\n", kStartupReadyMarker);
            ::fflush(stdout);
        });
        if (!a.appDirArg.isEmpty())
            a.mainWindow->hideSetDataDirectory();
        if (Settings.drawMethod() != QSGRendererInterface::Vulkan)
            a.mainWindow->setProperty("windowOpacity", 0.0);
        a.mainWindow->show();
        a.processEvents();
        a.mainWindow->setFullScreen(a.isFullScreen);
        splash.finish(a.mainWindow);

        // Set up a local IPC server so the watchdog parent can forward
        // QFileOpenEvent file paths (e.g. files opened from Finder on macOS)
        // to this child process which holds the actual main window.
        QLocalServer fileOpenServer;
        QLocalServer::removeServer("shotcut-file-open");
        fileOpenServer.listen("shotcut-file-open");
        QObject::connect(&fileOpenServer, &QLocalServer::newConnection, [&]() {
            QLocalSocket *socket = fileOpenServer.nextPendingConnection();
            QObject::connect(socket, &QLocalSocket::readyRead, [socket, &a]() {
                QString filePath = QString::fromUtf8(socket->readAll());
                if (!filePath.isEmpty() && a.mainWindow)
                    a.mainWindow->openMultiple(QStringList() << filePath);
                socket->deleteLater();
            });
        });

        if (!a.resourceArg.isEmpty()) {
            QStringList ls;
            for (auto &s : a.resourceArg)
                ls << QFileInfo(QDir::currentPath(), s).filePath();
            a.mainWindow->openMultiple(ls);
        } else {
            a.mainWindow->open(a.mainWindow->untitledFileName());
        }

        result = a.exec();

        if (EXIT_RESTART == result || EXIT_RESET == result) {
            LOG_DEBUG() << "restarting app";
            ::qunsetenv("QT_QUICK_CONTROLS_CONF"); // See MainWindow::changeTheme()
            QProcess *restart = new QProcess;
            QStringList args = a.arguments();
            if (!args.isEmpty())
                args.removeFirst();
            restart->start(a.applicationFilePath(), args, QIODevice::NotOpen);
            result = EXIT_SUCCESS;
        }

    } else {
        // Run as a parent process to check if the child crashes on startup
#ifdef Q_OS_MAC
        macosHideFromDock();
#endif
        QProcess child;
        QStringList args = a.arguments();
        if (!args.isEmpty())
            args.removeFirst();
        // Process any pending events to catch early QFileOpenEvents from macOS.
        a.processEvents();
        // If a file-open event arrived before the child starts, include it now.
        if (!a.fileOpenEventArgs.isEmpty())
            args << a.fileOpenEventArgs;
        ::qputenv(kWatchdogEnvVar, "1");
        child.setProcessChannelMode(QProcess::MergedChannels);
        // Set by the readyRead handler once kStartupReadyMarker is seen in the child's output.
        bool startupConfirmed = false;
        // Accumulates recent output in case the marker straddles two reads.
        QByteArray outputSearchBuffer;
        QObject::connect(&child, &QProcess::readyRead, [&]() {
            QByteArray output = child.readAll();
            if (output.isEmpty())
                return;
            if (!startupConfirmed) {
                outputSearchBuffer.append(output);
                if (outputSearchBuffer.contains(kStartupReadyMarker)) {
                    startupConfirmed = true;
                    output.replace(QByteArray(kStartupReadyMarker) + '\n', "");
                }
                if (outputSearchBuffer.size() > 4096)
                    outputSearchBuffer = outputSearchBuffer.right(4096);
            }
            if (!output.isEmpty()) {
                ::fputs(output.constData(), stdout);
                ::fflush(stdout);
            }
        });

        struct ChildRunResult
        {
            qint64 elapsedMs;
            bool hung;
        };

        auto runChildAndWait = [&](const char *backend) -> ChildRunResult {
#ifdef Q_OS_WIN
            ::qputenv("QSG_RHI_BACKEND", backend);
#else
            Q_UNUSED(backend);
#endif
            startupConfirmed = false;
            outputSearchBuffer.clear();
            QElapsedTimer timer;
            timer.start();
            child.start(a.applicationFilePath(), args, QIODevice::ReadOnly);
            if (!child.waitForStarted()) {
                LOG_WARNING() << "child process failed to start";
                return {timer.elapsed(), false};
            }
            bool hung = false;
            QTimer hangTimer;
            hangTimer.setSingleShot(true);
            QObject::connect(&hangTimer, &QTimer::timeout, [&]() {
                if (!startupConfirmed) {
                    hung = true;
                    LOG_WARNING() << "child process appears hung during startup; terminating";
                    child.kill();
                }
            });
            hangTimer.start(kStartupHangTimeoutMs);
            QEventLoop loop;
            QObject::connect(&child, &QProcess::finished, &loop, &QEventLoop::quit);
            loop.exec();
            hangTimer.stop();
            // Flush any output not yet delivered via readyRead.
            const QByteArray remaining = child.readAll();
            if (!remaining.isEmpty()) {
                ::fputs(remaining.constData(), stdout);
                ::fflush(stdout);
            }
            return {timer.elapsed(), hung};
        };

        const auto firstRun = runChildAndWait(
            (qEnvironmentVariableIsSet("QSG_RHI_BACKEND") ? qgetenv("QSG_RHI_BACKEND") : "d3d11"));
        const bool firstRunCrashed = QProcess::CrashExit == child.exitStatus() || child.exitCode();
        if (firstRun.hung || (firstRunCrashed && firstRun.elapsedMs <= kWatchdogTimeoutMs)) {
            LOG_WARNING() << (firstRun.hung
                                  ? "child process hung during startup, restarting in safe mode"
                                  : "child process failed, restarting in safe mode");
            Settings.setSafeMode(true);
            Settings.sync();
            runChildAndWait("opengl");
        }
    }

    return result;
}
