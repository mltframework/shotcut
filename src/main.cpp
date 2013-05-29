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

#include <QtGui>
#include "mainwindow.h"
#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

class Application : public QApplication
{
public:
    MainWindow* mainWindow;
    QTranslator qtTranslator;
    QTranslator shotcutTranslator;
    QString resourceArg;

    Application(int &argc, char **argv)
        : QApplication(argc, argv)
    {
        QDir dir(applicationDirPath());
        dir.cd("lib");
        dir.cd("qt4");
        addLibraryPath(dir.absolutePath());
        setOrganizationName("Meltytech");
        setOrganizationDomain("meltytech.com");
        setApplicationName("Shotcut");
        setApplicationVersion(SHOTCUT_VERSION);
#if defined(Q_WS_MAC)
        setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

        // Load translations
        const QString locale = QLocale::system().name();
        dir = applicationDirPath();
    #if defined(Q_WS_MAC)
        dir.cdUp();
        dir.cd("Resources");
        dir.cd("translations");
    #elif defined(Q_WS_WIN)
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
        if (shotcutTranslator.load("shotcut_" + locale, dir.absolutePath()))
            installTranslator(&shotcutTranslator);
        if (argc > 1)
            resourceArg = QString::fromUtf8(argv[1]);
    }

    ~Application()
    {
        delete mainWindow;
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
#ifdef Q_WS_X11
    XInitThreads();
#endif
    Application a(argc, argv);
    QSplashScreen splash(QPixmap(":/icons/icons/shotcut-logo-640.png"));
    splash.showMessage(QCoreApplication::translate("", "Loading plugins..."), Qt::AlignHCenter | Qt::AlignBottom);
    splash.show();
    a.mainWindow = &MAIN;
    a.mainWindow->show();
    splash.finish(a.mainWindow);
    if (!a.resourceArg.isEmpty())
        a.mainWindow->open(a.resourceArg);
    return a.exec();
}
