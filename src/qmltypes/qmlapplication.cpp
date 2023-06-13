/*
 * Copyright (c) 2013-2023 Meltytech, LLC
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
#include "videowidget.h"
#include "settings.h"
#include "util.h"
#include <QApplication>
#include <QSysInfo>
#include <QCursor>
#include <QPalette>
#include <QStyle>
#include <QFileInfo>
#include <QMessageBox>
#include <QCheckBox>
#include <QClipboard>
#ifdef Q_OS_WIN
#include <QLocale>
#else
#include <clocale>
#endif
#include <limits>

QmlApplication &QmlApplication::singleton()
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
#ifdef Q_OS_MAC
    return Qt::WindowModal;
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
#if defined(Q_OS_MAC)
    return "macOS";
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
    QScopedPointer<Mlt::Producer> producer(new Mlt::Producer(
                                               MAIN.filterController()->attachedModel()->producer()));
    MLT.copyFilters(producer.data());
    QGuiApplication::clipboard()->setText(MLT.filtersClipboardXML());
    emit QmlApplication::singleton().filtersCopied();
}

void QmlApplication::pasteFilters()
{
    QScopedPointer<Mlt::Producer> producer(new Mlt::Producer(
                                               MAIN.filterController()->attachedModel()->producer()));
    if (confirmOutputFilter()) {
        QString s = QGuiApplication::clipboard()->text();
        if (MLT.isMltXml(s)) {
            Mlt::Profile profile(kDefaultMltProfile);
            Mlt::Producer filtersProducer(profile, "xml-string", s.toUtf8().constData());
            if (filtersProducer.is_valid() && filtersProducer.filter_count() > 0
                    && filtersProducer.get_int(kShotcutFiltersClipboard)) {
                MLT.pasteFilters(producer.get(), &filtersProducer);
            } else {
                MLT.pasteFilters(producer.data());
            }
        } else {
            MLT.pasteFilters(producer.data());
        }
        emit QmlApplication::singleton().filtersPasted(
            MAIN.filterController()->attachedModel()->producer());
    }
}

QString QmlApplication::timecode(int frames)
{
    if (MLT.producer() && MLT.producer()->is_valid())
        return MLT.producer()->frames_to_time(frames, mlt_time_smpte_df);
    else
        return QString();
}

int QmlApplication::audioChannels()
{
    return MLT.audioChannels();
}

QString QmlApplication::getNextProjectFile(const QString &filename)
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

bool QmlApplication::isProjectFolder()
{
    QDir dir(MLT.projectFolder());
    return (!MLT.projectFolder().isEmpty() && dir.exists());
}

qreal QmlApplication::devicePixelRatio()
{
    return MAIN.devicePixelRatioF();
}

void QmlApplication::showStatusMessage(const QString &message, int timeoutSeconds)
{
    MAIN.showStatusMessage(message, timeoutSeconds);
}

int QmlApplication::maxTextureSize()
{
    auto *videoWidget = qobject_cast<Mlt::VideoWidget *>(MLT.videoWidget());
    return videoWidget ? videoWidget->maxTextureSize() : 0;
}

bool QmlApplication::confirmOutputFilter()
{
    bool result = true;
    QScopedPointer<Mlt::Producer> producer(new Mlt::Producer(
                                               MAIN.filterController()->attachedModel()->producer()));
    if (producer->is_valid()
            && mlt_service_tractor_type == producer->type()
            && !producer->get(kShotcutTransitionProperty)
            && MAIN.filterController()->attachedModel()->rowCount() == 0
            && Settings.askOutputFilter()) {
        QMessageBox dialog(QMessageBox::Warning,
                           qApp->applicationName(),
                           tr("<p>Do you really want to add filters to <b>Output</b>?</p>"
                              "<p><b>Timeline > Output</b> is currently selected. "
                              "Adding filters to <b>Output</b> affects ALL clips in the "
                              "timeline including new ones that will be added.</p>"),
                           QMessageBox::No | QMessageBox::Yes, &MAIN);
        dialog.setWindowModality(dialogModality());
        dialog.setDefaultButton(QMessageBox::No);
        dialog.setEscapeButton(QMessageBox::Yes);
        dialog.setCheckBox(new QCheckBox(tr("Do not show this anymore.", "confirm output filters dialog")));
        result = dialog.exec() == QMessageBox::Yes;
        if (dialog.checkBox()->isChecked()) {
            Settings.setAskOutputFilter(false);
        }
    }
    return result;
}

QDir QmlApplication::dataDir()
{
    QDir dir(qApp->applicationDirPath());
#if defined(Q_OS_MAC)
    dir.cdUp();
    dir.cd("Resources");
#else
#if defined(Q_OS_UNIX) || (defined(Q_OS_WIN) && defined(NODEPLOY))
    dir.cdUp();
#endif
    dir.cd("share");
#endif
    return dir;
}

QColor QmlApplication::contrastingColor(QString color)
{
    return Util::textColor(color);
}

QStringList QmlApplication::wipes()
{
    QStringList result;
    const auto transitions = QString::fromLatin1("transitions");
    QDir dir(Settings.appDataLocation());
    if (!dir.exists(transitions)) {
        dir.mkdir(transitions);
    }
    if (dir.cd(transitions)) {
        for (auto &s : dir.entryList(QDir::Files | QDir::Readable)) {
            result << dir.filePath(s);
        }
    }
    return result;
}

bool QmlApplication::addWipe(const QString &filePath)
{
    const auto transitions = QString::fromLatin1("transitions");
    QDir dir(Settings.appDataLocation());
    if (!dir.exists(transitions)) {
        dir.mkdir(transitions);
    }
    if (dir.cd(transitions)) {
        return QFile::copy(filePath, dir.filePath(QFileInfo(filePath).fileName()));
    }
    return false;
}
