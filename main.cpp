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

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDir dir(QApplication::applicationDirPath());
    dir.cd("lib");
    dir.cd("qt4");
    QCoreApplication::addLibraryPath(dir.absolutePath());
    a.setOrganizationName("Meltytech");
    a.setOrganizationDomain("meltytech.com");
    a.setApplicationName("Shotcut");
    a.setApplicationVersion(SHOTCUT_VERSION);
    QSplashScreen splash(QPixmap(":/icons/icons/shotcut-logo-640.png"));
    splash.showMessage(a.tr("Loading plugins..."), Qt::AlignHCenter | Qt::AlignBottom);
    splash.show();
    MainWindow w;
    splash.finish(&w);
    w.show();
    if (argc > 1)
        w.open(argv[1]);
    return a.exec();
}
