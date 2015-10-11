/*
 * Copyright (c) 2011-2015 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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
    Logger::write(cuteLoggerLevel, __FILE__, __LINE__, "MLT", message);
}

class Application : public QApplication
{
public:
    MainWindow* mainWindow;
    QTranslator qtTranslator;
    QTranslator qtBaseTranslator;
    QTranslator shotcutTranslator;
    QString resourceArg;
    bool isFullScreen;

    Application(int &argc, char **argv)
        : QApplication(argc, argv)
    {
        QDir dir(applicationDirPath());
        dir.cd("lib");
        dir.cd("qt5");
        addLibraryPath(dir.absolutePath());
        setOrganizationName("Meltytech");
        setOrganizationDomain("meltytech.com");
        setApplicationName("Shotcut");
        setApplicationVersion(SHOTCUT_VERSION);
        setAttribute(Qt::AA_UseHighDpiPixmaps);
#if defined(Q_OS_MAC)
        setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
        setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
#if defined(Q_OS_WIN)
        if (Settings.playerGPU()) {
            QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
        } else if (Settings.drawMethod() >= Qt::AA_UseDesktopOpenGL &&
                   Settings.drawMethod() <= Qt::AA_UseSoftwareOpenGL) {
            QCoreApplication::setAttribute(Qt::ApplicationAttribute(Settings.drawMethod()));
        }
#endif

        // Startup logging.
        dir = QStandardPaths::standardLocations(QStandardPaths::DataLocation).first();
        if (!dir.exists()) dir.mkpath(dir.path());
        const QString logFileName = dir.filePath("shotcut-log.txt");
        QFile::remove(logFileName);
        FileAppender* fileAppender = new FileAppender(logFileName);
        fileAppender->setFormat("[%-7l] <%C> %m\n");
        Logger::registerAppender(fileAppender);
#ifndef NDEBUG
        // Only log to console in dev debug builds.
        ConsoleAppender* consoleAppender = new ConsoleAppender();
        consoleAppender->setFormat(fileAppender->format());
        Logger::registerAppender(consoleAppender);

        mlt_log_set_level(MLT_LOG_VERBOSE);
#else
        mlt_log_set_level(MLT_LOG_INFO);
#endif
        mlt_log_set_callback(mlt_log_handler);

        // Log some basic info.
        LOG_INFO() << "Starting Shotcut version" << SHOTCUT_VERSION;
#if defined (Q_OS_WIN)
        LOG_INFO() << "Windows version" << QSysInfo::windowsVersion();
#elif defined(Q_OS_MAC)
        LOG_INFO() << "OS X version" << QSysInfo::macVersion();
#else
        LOG_INFO() << "Linux version";
#endif
        LOG_INFO() << "number of logical cores =" << QThread::idealThreadCount();
        LOG_INFO() << "locale =" << QLocale();
        LOG_INFO() << "install dir =" <<  applicationDirPath();

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

        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addVersionOption();
        QCommandLineOption fullscreenOption("fullscreen",
            QCoreApplication::translate("main", "Fill the screen with the Shotcut window."));
        parser.addOption(fullscreenOption);
        QCommandLineOption gpuOption("gpu",
            QCoreApplication::translate("main", "Use GPU processing."));
        parser.addOption(gpuOption);
        parser.addPositionalArgument("resource",
            QCoreApplication::translate("main", "A file to open."));
        parser.process(arguments());
        isFullScreen = parser.isSet(fullscreenOption);
        if (parser.isSet(gpuOption))
            Settings.setPlayerGPU(true);
        if (!parser.positionalArguments().isEmpty())
            resourceArg = parser.positionalArguments().first();
    }

    ~Application()
    {
        delete mainWindow;
        qDebug() << "exiting";
    }

protected:
    bool event(QEvent *event) {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent*>(event);
            resourceArg = openEvent->file();
            return true;
        }
        else return QApplication::event(event);
    }
};

int main(int argc, char **argv)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#endif
    Application a(argc, argv);
    QSplashScreen splash(QPixmap(":/icons/shotcut-logo-640.png"));
    splash.showMessage(QCoreApplication::translate("main", "Loading plugins..."), Qt::AlignHCenter | Qt::AlignBottom);
    splash.show();

    a.setProperty("system-style", a.style()->objectName());
    MainWindow::changeTheme(Settings.theme());

    a.mainWindow = &MAIN;
    a.mainWindow->show();
    a.mainWindow->setFullScreen(a.isFullScreen);
    splash.finish(a.mainWindow);

    if (!a.resourceArg.isEmpty())
        a.mainWindow->open(a.resourceArg);

    int result = a.exec();

    if (EXIT_RESTART == result) {
        qDebug() << "restarting app";
        QProcess* restart = new QProcess;
        restart->start(a.applicationFilePath(), QStringList());
        restart->waitForReadyRead();
        restart->waitForFinished(1000);
        result = EXIT_SUCCESS;
    }
    return result;
}
