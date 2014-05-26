/*
 * Copyright (c) 2011 Meltytech, LLC
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
#include "mainwindow.h"
#include "settings.h"
#include <Logger.h>
#include <FileAppender.h>
#include <ConsoleAppender.h>

class Application : public QApplication
{
public:
    MainWindow* mainWindow;
    QTranslator qtTranslator;
    QTranslator qtBaseTranslator;
    QTranslator shotcutTranslator;
    QString resourceArg;

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
#endif
        LOG_INFO() << "Starting Shotcut version" << SHOTCUT_VERSION;
        LOG_INFO() << "locale =" << QLocale();

        // Load translations
        const QString locale = Settings.language();
        QLocale::setDefault(QLocale(locale));

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
        if (qtTranslator.load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
            installTranslator(&qtTranslator);
        else if (qtTranslator.load("qt_" + locale, dir.absolutePath()))
            installTranslator(&qtTranslator);
        if (qtBaseTranslator.load("qtbase_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
            installTranslator(&qtBaseTranslator);
        else if (qtBaseTranslator.load("qtbase_" + locale, dir.absolutePath()))
            installTranslator(&qtBaseTranslator);
        if (shotcutTranslator.load("shotcut_" + locale, dir.absolutePath()))
            installTranslator(&shotcutTranslator);
        if (argc > 1)
            resourceArg = QString::fromUtf8(argv[1]);
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
#endif
    Application a(argc, argv);
    QSplashScreen splash(QPixmap(":/icons/shotcut-logo-640.png"));
    splash.showMessage(QCoreApplication::translate("", "Loading plugins..."), Qt::AlignHCenter | Qt::AlignBottom);
    splash.show();

    a.setProperty("system-style", a.style()->objectName());
    MainWindow::changeTheme(Settings.theme());

    a.mainWindow = &MAIN;
    a.mainWindow->show();
    splash.finish(a.mainWindow);
    if (!a.resourceArg.isEmpty())
        a.mainWindow->open(a.resourceArg);
    return a.exec();
}
