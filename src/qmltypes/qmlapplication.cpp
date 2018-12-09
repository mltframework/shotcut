/*
 * Copyright (c) 2013-2018 Meltytech, LLC
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

#include "qmlapplication.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "controllers/filtercontroller.h"
#include "models/attachedfiltersmodel.h"
#include <QApplication>
#include <QSysInfo>
#include <QCursor>
#include <QPalette>
#include <QStyle>
#include <QFileInfo>
#ifdef Q_OS_WIN
#include <QLocale>
#else
#include <clocale>
#endif
#include <limits>

QmlApplication& QmlApplication::singleton()
{
    static QmlApplication instance;
    return instance;
}

QmlApplication::QmlApplication() :
    QObject()
{
}

Qt::WindowModality QmlApplication::dialogModality()
{
#ifdef Q_OS_OSX
    return (QSysInfo::macVersion() >= QSysInfo::MV_10_8)? Qt::WindowModal : Qt::ApplicationModal;
#else
    return Qt::ApplicationModal;
#endif
}

QPoint QmlApplication::mousePos()
{
    return QCursor::pos();
}

QColor QmlApplication::toolTipBaseColor()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if ("gtk+" == QApplication::style()->objectName())
        return QApplication::palette().highlight().color();
#endif
    return QApplication::palette().toolTipBase().color();
}

QColor QmlApplication::toolTipTextColor()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if ("gtk+" == QApplication::style()->objectName())
        return QApplication::palette().highlightedText().color();
#endif
    return QApplication::palette().toolTipText().color();
}

QString QmlApplication::OS()
{
#if defined(Q_OS_OSX)
    return "OS X";
#elif defined(Q_OS_LINUX)
    return "Linux";
#elif defined(Q_OS_UNIX)
    return "UNIX";
#elif defined(Q_OS_WIN)
    return "Windows";
#else
    return "";
#endif
}

QRect QmlApplication::mainWinRect()
{
    return MAIN.geometry();
}

bool QmlApplication::hasFiltersOnClipboard()
{
    return MLT.hasFiltersOnClipboard();
}

void QmlApplication::copyFilters()
{
    QScopedPointer<Mlt::Producer> producer(new Mlt::Producer(MAIN.filterController()->attachedModel()->producer()));
    MLT.copyFilters(producer.data());
    emit QmlApplication::singleton().filtersCopied();
}

void QmlApplication::pasteFilters()
{
    QScopedPointer<Mlt::Producer> producer(new Mlt::Producer(MAIN.filterController()->attachedModel()->producer()));
    MLT.pasteFilters(producer.data());
    emit QmlApplication::singleton().filtersPasted(MAIN.filterController()->attachedModel()->producer());
}

QString QmlApplication::timecode(int frames)
{
    if (MLT.producer() && MLT.producer()->is_valid())
        return MLT.producer()->frames_to_time(frames, mlt_time_smpte);
    else
        return QString();
}

int QmlApplication::audioChannels()
{
    return MLT.audioChannels();
}

QString QmlApplication::getNextProjectFile(const QString& filename)
{
    QDir dir(MLT.projectFolder());
    if (!MLT.projectFolder().isEmpty() && dir.exists()) {
        QFileInfo info(filename);
        QString basename = info.completeBaseName();
        QString extension = info.suffix();
        if (extension.isEmpty()) {
            extension = basename;
            basename = QString();
        }
        for (unsigned i = 1; i < std::numeric_limits<unsigned>::max(); i++) {
            QString filename = QString::fromLatin1("%1%2.%3").arg(basename).arg(i).arg(extension);
            if (!dir.exists(filename))
                return dir.filePath(filename);
        }
    }
    return QString();
}

