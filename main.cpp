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

class Application : public QApplication
{
public:
    MainWindow* mainWindow;

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
        mainWindow = &MAIN;
    }

    ~Application()
    {
        delete mainWindow;
    }

protected:
    bool event(QEvent *event) {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent*>(event);
            mainWindow->open(openEvent->file());
            return true;
        }
        else return QApplication::event(event);
    }
};

int main(int argc, char **argv)
{
    Application a(argc, argv);

    // Load translations
    qDebug() << "Qt TranslationsPath=" << QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    QTranslator qtTranslator;
    QTranslator shotcutTranslator;
    QDir appDir = a.applicationDirPath();
    const QString locale = QLocale::system().name();
#ifdef Q_WS_MAC
    appDir.cdUp();
    appDir.cd("Resources");
    appDir.cd("translations");
#else
    appDir.cd("share");
    appDir.cd("translations");
#endif
    if (qtTranslator.load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        a.installTranslator(&qtTranslator);
    else if (qtTranslator.load("qt_" + locale, appDir.absolutePath()))
        a.installTranslator(&qtTranslator);
    if (shotcutTranslator.load("shotcut_" + locale, appDir.absolutePath()))
        a.installTranslator(&shotcutTranslator);

    QSplashScreen splash(QPixmap(":/icons/icons/shotcut-logo-640.png"));
    splash.showMessage(a.tr("Loading plugins..."), Qt::AlignHCenter | Qt::AlignBottom);
    splash.show();
    splash.finish(a.mainWindow);
    a.mainWindow->show();
    if (argc > 1)
        a.mainWindow->open(argv[1]);
    return a.exec();
}
