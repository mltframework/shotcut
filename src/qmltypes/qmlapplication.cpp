/*
 * Copyright (c) 2013-2026 Meltytech, LLC
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

#include "actions.h"
#include "controllers/filtercontroller.h"
#include "dialogs/addonfiltersdialog.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/attachedfiltersmodel.h"
#include "settings.h"
#include "util.h"
#include "videowidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCursor>
#include <QFileInfo>
#include <QMessageBox>
#include <QPalette>
#include <QStyle>
#include <QSysInfo>
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

/*!
    \qmltype Application
    \inqmlmodule org.shotcut.qml
    \brief Application-wide utilities, accessed via the \c application context property.

    \c application is set as a context property on every Shotcut QML view.
    It cannot be instantiated from QML — use the global \c application identifier directly:

    \code
    application.showStatusMessage("Ready")
    var t = application.clockFromFrames(150)
    var c = application.contrastingColor("red")
    \endcode
*/

/*!
    \qmlsignal Application::paletteChanged()
    \brief Emitted when the application color palette changes (e.g. dark/light mode switch).
*/

/*!
    \qmlsignal Application::filtersCopied()
    \brief Emitted when filters are copied to the clipboard.
*/

/*!
    \qmlsignal Application::filtersPasted()
    \brief Emitted when filters are pasted from the clipboard.
*/

/*!
    \qmlproperty Qt::WindowModality Application::dialogModality
    \brief The modality to use for dialogs launched from QML.
*/

QmlApplication::QmlApplication()
    : QObject()
{}

Qt::WindowModality QmlApplication::dialogModality()
{
#ifdef Q_OS_MAC
    return Qt::WindowModal;
#else
    return Qt::ApplicationModal;
#endif
}

/*!
    \qmlproperty point Application::mousePos
    \brief The current global cursor screen coordinates.
*/

QPoint QmlApplication::mousePos()
{
    return QCursor::pos();
}

/*!
    \qmlproperty color Application::toolTipBaseColor
    \brief The background color of tooltips, derived from the current palette.
    Notifies \l paletteChanged.
*/

QColor QmlApplication::toolTipBaseColor()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if ("gtk+" == QApplication::style()->objectName())
        return QApplication::palette().highlight().color();
#endif
    return QApplication::palette().toolTipBase().color();
}

/*!
    \qmlproperty color Application::toolTipTextColor
    \brief The text color of tooltips, derived from the current palette.
    Notifies \l paletteChanged.
*/

QColor QmlApplication::toolTipTextColor()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if ("gtk+" == QApplication::style()->objectName())
        return QApplication::palette().highlightedText().color();
#endif
    return QApplication::palette().toolTipText().color();
}

/*!
    \qmlproperty string Application::OS
    \brief The operating system name: \c "Windows", \c "Linux", or \c "macOS".
*/

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

/*!
    \qmlproperty rect Application::mainWinRect
    \brief The geometry of the main application window.
*/

QRect QmlApplication::mainWinRect()
{
    return MAIN.geometry();
}

/*!
    \qmlproperty bool Application::hasFiltersOnClipboard
    \brief Whether the clipboard currently contains copied filters.
    Notifies \l filtersCopied.
*/

bool QmlApplication::hasFiltersOnClipboard()
{
    return MLT.hasFiltersOnClipboard();
}

/*!
    \qmlmethod Application::copyEnabledFilters()
    \brief Copies only the checked (enabled) filters to the clipboard.
*/

void QmlApplication::copyEnabledFilters()
{
    QScopedPointer<Mlt::Producer> producer(
        new Mlt::Producer(MAIN.filterController()->attachedModel()->producer()));
    MLT.copyFilters(producer.data(), MLT.FILTER_INDEX_ENABLED);
    QGuiApplication::clipboard()->setText(MLT.filtersClipboardXML());
    emit QmlApplication::singleton().filtersCopied();
}

/*!
    \qmlmethod Application::copyAllFilters()
    \brief Copies all filters on the currently selected clip to the clipboard.
*/

void QmlApplication::copyAllFilters()
{
    QScopedPointer<Mlt::Producer> producer(
        new Mlt::Producer(MAIN.filterController()->attachedModel()->producer()));
    MLT.copyFilters(producer.data(), MLT.FILTER_INDEX_ALL);
    QGuiApplication::clipboard()->setText(MLT.filtersClipboardXML());
    emit QmlApplication::singleton().filtersCopied();
}

/*!
    \qmlmethod Application::copyCurrentFilter()
    \brief Copies the currently selected filter to the clipboard.
*/

void QmlApplication::copyCurrentFilter()
{
    int currentIndex = MAIN.filterController()->currentIndex();
    if (currentIndex < 0) {
        MAIN.showStatusMessage(tr("Select a filter to copy"));
        return;
    }
    QScopedPointer<Mlt::Producer> producer(
        new Mlt::Producer(MAIN.filterController()->attachedModel()->producer()));
    MLT.copyFilters(producer.data(), currentIndex);
    QGuiApplication::clipboard()->setText(MLT.filtersClipboardXML());
    emit QmlApplication::singleton().filtersCopied();
}

/*!
    \qmlmethod string Application::clockFromFrames(int frames)
    \brief Converts a frame number \a frames to a clock string (\c HH:MM:SS.mmm or \c HH:MM:SS:FF) at the current profile fps.
*/

QString QmlApplication::clockFromFrames(int frames)
{
    if (MLT.producer()) {
        return MLT.producer()->frames_to_time(frames, Settings.timeFormat());
    }
    return QString();
}

/*!
    \qmlmethod string Application::timeFromFrames(int frames)
    \brief Converts a frame number \a frames to an MLT timecode string.
*/

QString QmlApplication::timeFromFrames(int frames)
{
    if (MLT.producer()) {
        return MLT.producer()->frames_to_time(frames, Settings.timeFormat());
    }
    return QString();
}

/*!
    \qmlmethod int Application::audioChannels()
    \brief Returns the number of audio channels in the current project.
*/

int QmlApplication::audioChannels()
{
    return MLT.audioChannels();
}

/*!
    \qmlmethod string Application::getNextProjectFile(string filename)
    \brief Returns a versioned file path for saving a new project-related file
    named \a filename (e.g. \c project-001.txt) next to the current project file.
*/

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

/*!
    \qmlmethod bool Application::isProjectFolder()
    \brief Returns \c true if the current project is saved inside a project folder.
*/

bool QmlApplication::isProjectFolder()
{
    QDir dir(MLT.projectFolder());
    return (!MLT.projectFolder().isEmpty() && dir.exists());
}

/*!
    \qmlproperty real Application::devicePixelRatio
    \brief The display device pixel ratio (e.g. 2.0 on HiDPI screens).
*/

qreal QmlApplication::devicePixelRatio()
{
    return MAIN.devicePixelRatioF();
}

/*!
    \qmlmethod Application::showStatusMessage(string message, int timeoutSeconds = 15)
    \brief Displays \a message in the main window status bar for \a timeoutSeconds seconds.
*/

void QmlApplication::showStatusMessage(const QString &message, int timeoutSeconds)
{
    MAIN.showStatusMessage(message, timeoutSeconds);
}

/*!
    \qmlmethod Application::showAddOnFiltersDialog()
    \brief Opens the Add-On Filters catalog dialog.
*/

void QmlApplication::showAddOnFiltersDialog()
{
    auto *controller = MAIN.filterController();
    if (!controller)
        return;

    AddOnFiltersDialog dialog(controller->addOnServiceModel(), &MAIN);
    dialog.setWindowModality(dialogModality());
    dialog.exec();
}

/*!
    \qmlproperty int Application::maxTextureSize
    \brief The maximum supported GPU texture dimension in pixels.
*/

int QmlApplication::maxTextureSize()
{
    auto *videoWidget = qobject_cast<Mlt::VideoWidget *>(MLT.videoWidget());
    return videoWidget ? videoWidget->maxTextureSize() : 0;
}

/*!
    \qmlmethod bool Application::confirmOutputFilter()
    \brief Prompts the user to confirm applying a filter to the timeline output.
    Returns \c true if the user confirms.
*/

bool QmlApplication::confirmOutputFilter()
{
    bool result = true;
    if (MAIN.filterController()->isOutputTrackSelected() && Settings.askOutputFilter()) {
        QMessageBox dialog(QMessageBox::Warning,
                           qApp->applicationName(),
                           tr("<p>Do you really want to add filters to <b>Output</b>?</p>"
                              "<p><b>Timeline > Output</b> is currently selected. "
                              "Adding filters to <b>Output</b> affects ALL clips in the "
                              "timeline including new ones that will be added.</p>"),
                           QMessageBox::No | QMessageBox::Yes,
                           &MAIN);
        dialog.setWindowModality(dialogModality());
        dialog.setDefaultButton(QMessageBox::No);
        dialog.setEscapeButton(QMessageBox::Yes);
        dialog.setCheckBox(
            new QCheckBox(tr("Do not show this anymore.", "confirm output filters dialog")));
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

/*!
    \qmlmethod color Application::contrastingColor(string color)
    \brief Returns black or white, whichever contrasts better against \a color.
*/

QColor QmlApplication::contrastingColor(QString color)
{
    return Util::textColor(color);
}

/*!
    \qmlproperty list<string> Application::wipes
    \brief The list of available wipe transition file names.
*/

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

/*!
    \qmlmethod bool Application::addWipe(string filePath)
    \brief Adds a custom wipe transition image at \a filePath to the wipes list.
    Returns \c true on success.
*/

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

/*!
    \qmlmethod bool Application::intersects(rect a, rect b)
    \brief Returns \c true if rectangle \a a intersects rectangle \a b.
*/

bool QmlApplication::intersects(const QRectF &a, const QRectF &b)
{
    return a.intersects(b);
}

/*!
    \qmlmethod string Application::actionFirstShortcut(string actionName)
    \brief Returns the first keyboard shortcut string for the action identified by \a actionName,
    or an empty string if none is assigned.
*/

QString QmlApplication::actionFirstShortcut(const QString &actionName)
{
    const auto a = Actions[actionName];
    return (a && !a->shortcut().isEmpty())
               ? QStringLiteral(" (%1)").arg(a->shortcut().toString(QKeySequence::NativeText))
               : QString();
}
