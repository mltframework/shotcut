/*
 * Copyright (c) 2011-2018 Meltytech, LLC
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

#ifdef Q_OS_WIN
#ifdef QT_DEBUG
#   include <exchndl.h>
#endif
extern "C"
{
    // Inform the driver we could make use of the discrete gpu
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
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
    mlt_properties properties = service? MLT_SERVICE_PROPERTIES((mlt_service) service) : NULL;
    if (properties) {
        char *mlt_type = mlt_properties_get(properties, "mlt_type");
        char *service_name = mlt_properties_get(properties, "mlt_service");
        char *resource = mlt_properties_get(properties, "resource");
        if (!resource || resource[0] != '<' || resource[strlen(resource) - 1] != '>')
            mlt_type = mlt_properties_get(properties, "mlt_type" );
        if (service_name)
            message = QString("[%1 %2] ").arg(mlt_type).arg(service_name);
        else
            message = QString().sprintf("[%s %p] ", mlt_type, service);
        if (resource)
            message.append(QString("\"%1\" ").arg(resource));
        message.append(QString().vsprintf(format, args));
        message.replace('\n', "");
    } else {
        message = QString().vsprintf(format, args);
        message.replace('\n', "");
    }
    cuteLogger->write(cuteLoggerLevel, __FILE__, __LINE__, "MLT",
                      cuteLogger->defaultCategory().toLatin1().constData(), message);
}

class Application : public QApplication
{
public:
    MainWindow* mainWindow;
    QTranslator qtTranslator;
    QTranslator qtBaseTranslator;
    QTranslator shotcutTranslator;
    QStringList resourceArg;
    bool isFullScreen;
    QString appDirArg;

    Application(int &argc, char **argv)
        : QApplication(argc, argv)
    {
        QDir dir(applicationDirPath());
#ifdef Q_OS_MAC
        dir.cdUp();
        dir.cd("PlugIns");
        dir.cd("qt");
#else
        dir.cd("lib");
        dir.cd("qt5");
#endif
        addLibraryPath(dir.absolutePath());
        setOrganizationName("Meltytech");
        setOrganizationDomain("meltytech.com");
        setApplicationName("Shotcut");
        setApplicationVersion(SHOTCUT_VERSION);
        setAttribute(Qt::AA_UseHighDpiPixmaps);
        setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#if defined(Q_OS_MAC)
        setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

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
        parser.addPositionalArgument("[FILE]...",
            QCoreApplication::translate("main", "Zero or more files or folders to open"));
        parser.process(arguments());
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
        dir = Settings.appDataLocation();
        if (!dir.exists()) dir.mkpath(dir.path());
        const QString logFileName = dir.filePath("shotcut-log.txt");
        QFile::remove(logFileName);
        FileAppender* fileAppender = new FileAppender(logFileName);
        fileAppender->setFormat("[%{type:-7}] <%{function}> %{message}\n");
        cuteLogger->registerAppender(fileAppender);
#ifndef NDEBUG
        // Only log to console in dev debug builds.
        ConsoleAppender* consoleAppender = new ConsoleAppender();
        consoleAppender->setFormat(fileAppender->format());
        cuteLogger->registerAppender(consoleAppender);

        mlt_log_set_level(MLT_LOG_VERBOSE);
#else
        mlt_log_set_level(MLT_LOG_INFO);
#endif
        mlt_log_set_callback(mlt_log_handler);
        cuteLogger->logToGlobalInstance("qml", true);

        // Log some basic info.
        LOG_INFO() << "Starting Shotcut version" << SHOTCUT_VERSION;
#if defined (Q_OS_WIN)
        LOG_INFO() << "Windows version" << QSysInfo::windowsVersion();
#elif defined(Q_OS_MAC)
        LOG_INFO() << "macOS version" << QSysInfo::macVersion();
#else
        LOG_INFO() << "Linux version";
#endif
        LOG_INFO() << "number of logical cores =" << QThread::idealThreadCount();
        LOG_INFO() << "locale =" << QLocale();
        LOG_INFO() << "install dir =" <<  applicationDirPath();
        LOG_INFO() << "device pixel ratio =" << devicePixelRatio();
        Settings.log();

#if defined(Q_OS_WIN)
        if (Settings.playerGPU()) {
            QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
        } else if (Settings.drawMethod() >= Qt::AA_UseDesktopOpenGL &&
                   Settings.drawMethod() <= Qt::AA_UseSoftwareOpenGL) {
            QCoreApplication::setAttribute(Qt::ApplicationAttribute(Settings.drawMethod()));
        }
#endif
        // Load translations
        QString locale = Settings.language();
        dir = applicationDirPath();
    #if defined(Q_OS_MAC)
        dir.cdUp();
        dir.cd("Resources");
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
        if (qtTranslator.load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
            installTranslator(&qtTranslator);
        else if (qtTranslator.load("qt_" + locale, dir.absolutePath()))
            installTranslator(&qtTranslator);
        if (qtBaseTranslator.load("qtbase_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
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
    bool event(QEvent *event) {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent*>(event);
            resourceArg << openEvent->file();
            return true;
        }
        else return QApplication::event(event);
    }
};

int main(int argc, char **argv)
{
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
    ExcHndlInit();
#endif
#if QT_VERSION >= 0x050600
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#ifdef Q_OS_MAC
    // Launcher and Spotlight on macOS are not setting this environment
    // variable needed by setlocale() as used by MLT.
    if (QProcessEnvironment::systemEnvironment().value("LC_NUMERIC").isEmpty()) {
        qputenv("LC_NUMERIC", QLocale().name().toUtf8());

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
#endif

    Application a(argc, argv);
    QSplashScreen splash(QPixmap(":/icons/shotcut-logo-320x320.png"));
    splash.showMessage(QCoreApplication::translate("main", "Loading plugins..."), Qt::AlignRight | Qt::AlignVCenter);
    splash.show();

    a.setProperty("system-style", a.style()->objectName());
    MainWindow::changeTheme(Settings.theme());

    a.mainWindow = &MAIN;
    if (!a.appDirArg.isEmpty())
        a.mainWindow->hideSetDataDirectory();
    a.mainWindow->show();
    a.mainWindow->setFullScreen(a.isFullScreen);
    splash.finish(a.mainWindow);

    if (!a.resourceArg.isEmpty())
        a.mainWindow->openMultiple(a.resourceArg);
    else
        a.mainWindow->open(a.mainWindow->untitledFileName());

    int result = a.exec();

    if (EXIT_RESTART == result) {
        LOG_DEBUG() << "restarting app";
        QProcess* restart = new QProcess;
        QStringList args = a.arguments();
        if (!args.isEmpty())
            args.removeFirst();
        restart->start(a.applicationFilePath(), args, QIODevice::NotOpen);
        result = EXIT_SUCCESS;
    }
    return result;
}
