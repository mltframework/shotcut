/*
 * Copyright (c) 2011-2024 Meltytech, LLC
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

#include <QtWidgets>
#include <QtGlobal>
#include "mainwindow.h"
#include "settings.h"
#include <Logger.h>
#include <FileAppender.h>
#include <ConsoleAppender.h>
#include <QSysInfo>
#include <QProcess>
#include <QCommandLineParser>
#include <framework/mlt_log.h>
#include <QFile>
#include <QQuickStyle>
#include <QQuickWindow>

#ifdef Q_OS_MAC
#include "macos.h"
#endif

#ifdef Q_OS_WIN
#ifdef QT_DEBUG
#   include <exchndl.h>
#endif
extern "C"
{
    // Inform the driver we could make use of the discrete gpu
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#endif

static const int kMaxCacheCount = 5000;

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
            mlt_type = mlt_properties_get(properties, "mlt_type" );
        if (service_name)
            message = QString("[%1 %2] ").arg(mlt_type, service_name);
        else
            message = QString::asprintf("[%s %p] ", mlt_type, service);
        if (resource)
            message.append(QString("\"%1\" ").arg(resource));
        message.append(QString::vasprintf(format, args));
        message.replace('\n', "");
    } else {
        message = QString::vasprintf(format, args);
        message.replace('\n', "");
    }
    cuteLogger->write(cuteLoggerLevel, __FILE__, __LINE__, "MLT",
                      cuteLogger->defaultCategory().toLatin1().constData(), message);
}

class Application : public QApplication
{
public:
    MainWindow *mainWindow {nullptr};
    QTranslator qtTranslator;
    QTranslator qtBaseTranslator;
    QTranslator shotcutTranslator;
    QStringList resourceArg;
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
        QCommandLineOption fullscreenOption("fullscreen",
                                            QCoreApplication::translate("main", "Fill the screen with the Shotcut window."));
        parser.addOption(fullscreenOption);
#endif
        QCommandLineOption noupgradeOption("noupgrade",
                                           QCoreApplication::translate("main", "Hide upgrade prompt and menu item."));
        parser.addOption(noupgradeOption);
        QCommandLineOption glaxnimateOption("glaxnimate",
                                            QCoreApplication::translate("main", "Run Glaxnimate instead of Shotcut."));
        parser.addOption(glaxnimateOption);
        QCommandLineOption gpuOption("gpu",
                                     QCoreApplication::translate("main", "Use GPU processing."));
        parser.addOption(gpuOption);
        QCommandLineOption clearRecentOption("clear-recent",
                                             QCoreApplication::translate("main", "Clear Recent on Exit"));
        parser.addOption(clearRecentOption);
        QCommandLineOption appDataOption("appdata",
                                         QCoreApplication::translate("main", "The directory for app configuration and data."),
                                         QCoreApplication::translate("main", "directory"));
        parser.addOption(appDataOption);
        QCommandLineOption scaleOption("QT_SCALE_FACTOR",
                                       QCoreApplication::translate("main", "The scale factor for a high-DPI screen"),
                                       QCoreApplication::translate("main", "number"));
        parser.addOption(scaleOption);
        scaleOption = QCommandLineOption("QT_SCREEN_SCALE_FACTORS",
                                         QCoreApplication::translate("main", "A semicolon-separated list of scale factors for each screen"),
                                         QCoreApplication::translate("main", "list"));
        parser.addOption(scaleOption);
        QCommandLineOption scalePolicyOption("QT_SCALE_FACTOR_ROUNDING_POLICY",
                                             QCoreApplication::translate("main", "How to handle a fractional display scale: %1")
                                             .arg("Round, Ceil, Floor, RoundPreferFloor, PassThrough"),
                                             QCoreApplication::translate("main", "string"), "PassThrough");
        parser.addOption(scalePolicyOption);
#if defined(Q_OS_WIN)
        QCommandLineOption sdlAudioDriverOption("SDL_AUDIODRIVER",
                                                QCoreApplication::translate("main", "Which operating system audio API to use: %1")
                                                .arg("directsound, wasapi, winmm"),
                                                QCoreApplication::translate("main", "string"), "wasapi");
        parser.addOption(sdlAudioDriverOption);
#elif defined(Q_OS_LINUX)
        QCommandLineOption sdlAudioDriverOption("SDL_AUDIODRIVER",
                                                QCoreApplication::translate("main", "Which operating system audio API to use: %1")
                                                .arg("alsa, arts, dsp, esd, jack, pipewire, pulseaudio"),
                                                QCoreApplication::translate("main", "string"), "pulseaudio");
        parser.addOption(sdlAudioDriverOption);
#endif
        parser.addPositionalArgument("[FILE]...",
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
        if (!parser.value(appDataOption).isEmpty()) {
            appDirArg = parser.value(appDataOption);
            ShotcutSettings::setAppDataForSession(appDirArg);
        }
        if (parser.isSet(gpuOption))
            Settings.setPlayerGPU(true);
        if (!parser.positionalArguments().isEmpty())
            resourceArg = parser.positionalArguments();

        // Startup logging.
        dir.setPath(Settings.appDataLocation());
        if (!dir.exists()) dir.mkpath(dir.path());
        QFile::copy(dir.filePath("shotcut-log.txt"), dir.filePath("shotcut-log.bak"));
        const QString logFileName = dir.filePath("shotcut-log.txt");
        QFile::remove(logFileName);
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
#elif !defined(Q_OS_MAC)
        if (Settings.drawMethod() == Qt::AA_UseSoftwareOpenGL && !Settings.playerGPU()) {
            ::qputenv("LIBGL_ALWAYS_SOFTWARE", "1");
        }
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
            resourceArg << openEvent->file();
            return true;
        } else return QApplication::event(event);
    }
};

int main(int argc, char **argv)
{
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
    ExcHndlInit();
#endif
#ifndef QT_DEBUG
    ::qputenv("QT_LOGGING_RULES", "*.warning=false");
#endif
    for (int i = 1; i + 1 < argc; i++) {
        if (!::qstrcmp("--QT_SCALE_FACTOR", argv[i]) || !::qstrcmp("--QT_SCREEN_SCALE_FACTORS", argv[i])) {
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
#ifdef Q_OS_MAC
    // Launcher and Spotlight on macOS are not setting this environment
    // variable needed by setlocale() as used by MLT.
    if (QProcessEnvironment::systemEnvironment().value(MLT_LC_NAME).isEmpty()) {
        qputenv(MLT_LC_NAME, QLocale().name().toUtf8());

        QLocale localeByName(QLocale(QLocale().language(), QLocale().script(), QLocale().country()));
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
#endif

#if defined(Q_OS_WIN)
    // Windows can use Direct3D or OpenGL
#elif defined(Q_OS_MAC)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Metal);
    QCoreApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#else
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#endif

    Application a(argc, argv);
    int result = EXIT_SUCCESS;
#ifdef Q_OS_WIN
    if (::qEnvironmentVariableIsSet("QSG_RHI_BACKEND")) {
#endif
        QSplashScreen splash(QPixmap(":/icons/shotcut-logo-320x320.png"));

        // Log some basic info.
        LOG_INFO() << "Starting Shotcut version" << SHOTCUT_VERSION;
#if defined(Q_OS_WIN)
        LOG_INFO() << "Windows version" << QSysInfo::productVersion();
#elif defined(Q_OS_MAC)
        LOG_INFO() << "macOS version" << QSysInfo::productVersion();
#else
        LOG_INFO() << "Linux version" << QSysInfo::productVersion();;
#endif
        LOG_INFO() << "number of logical cores =" << QThread::idealThreadCount();
        LOG_INFO() << "locale =" << QLocale();
        LOG_INFO() << "install dir =" << a.applicationDirPath();
        Settings.log();

        // Expire old items from the qmlcache
        splash.showMessage(QCoreApplication::translate("main", "Expiring cache..."),
                           Qt::AlignRight | Qt::AlignVCenter);
        splash.show();
        a.processEvents();
        auto dir = QDir(QStandardPaths::standardLocations(QStandardPaths::CacheLocation).constFirst());
        if (dir.exists() && dir.cd("qmlcache")) {
            auto ls = dir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::Time);
            if (qMax(0, ls.size() - kMaxCacheCount) > 0) {
                LOG_INFO() << "removing" << qMax(0, ls.size() - kMaxCacheCount) << "from" << dir.path();
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

        splash.showMessage(QCoreApplication::translate("main", "Loading plugins..."),
                           Qt::AlignRight | Qt::AlignVCenter);
        a.processEvents();

        a.setProperty("system-style", a.style()->objectName());
        MainWindow::changeTheme(Settings.theme());
        QQuickStyle::setStyle("Fusion");

        a.mainWindow = &MAIN;
        if (!a.appDirArg.isEmpty())
            a.mainWindow->hideSetDataDirectory();
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
        a.mainWindow->setProperty("windowOpacity", 0.0);
#endif
        a.mainWindow->show();
        a.processEvents();
        a.mainWindow->setFullScreen(a.isFullScreen);
        splash.finish(a.mainWindow);

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
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
            ::qputenv("LIBGL_ALWAYS_SOFTWARE",
                      Settings.drawMethod() == Qt::AA_UseSoftwareOpenGL && !Settings.playerGPU()
                      ? "1" : "0");
#endif
            QProcess *restart = new QProcess;
            QStringList args = a.arguments();
            if (!args.isEmpty())
                args.removeFirst();
            restart->start(a.applicationFilePath(), args, QIODevice::NotOpen);
            result = EXIT_SUCCESS;
        }

#ifdef Q_OS_WIN
    } else { // if (::qEnvironmentVariableIsSet("QSG_RHI_BACKEND"))
        // Run as a parent process to check if the child crashes on startup
        QProcess *child = new QProcess;
        QStringList args = a.arguments();
        if (!args.isEmpty())
            args.removeFirst();
        ::qputenv("QSG_RHI_BACKEND", "d3d11");
        child->start(a.applicationFilePath(), args, QIODevice::NotOpen);
        child->waitForFinished();
        if (QProcess::CrashExit == child->exitStatus() || child->exitCode()) {
            LOG_WARNING() << "child process failed, restarting in OpenGL mode";
            ::qputenv("QSG_RHI_BACKEND", "opengl");
            child->start(a.applicationFilePath(), args, QIODevice::NotOpen);
        }
    }
#endif

    return result;
}
