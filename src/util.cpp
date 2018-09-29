/*
 * Copyright (c) 2014-2018 Meltytech, LLC
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

#include "util.h"
#include <QFileInfo>
#include <QWidget>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QUrl>
#include <QDesktopServices>
#include <QMessageBox>
#include <QMap>
#include <MltProducer.h>
#include "shotcut_mlt_properties.h"

QString Util::baseName(const QString &filePath)
{
    QString s = filePath;
    // Only if absolute path and not a URI.
    if (s.startsWith('/') || s.midRef(1, 2) == ":/" || s.midRef(1, 2) == ":\\")
        s = QFileInfo(s).fileName();
    return s;
}

void Util::setColorsToHighlight(QWidget* widget, QPalette::ColorRole role)
{
    QPalette palette = widget->palette();
    palette.setColor(role, palette.color(palette.Highlight));
    palette.setColor(role == QPalette::Button ? QPalette::ButtonText : QPalette::WindowText,
        palette.color(palette.HighlightedText));
    widget->setPalette(palette);
    widget->setAutoFillBackground(true);
}

void Util::showInFolder(const QString& path)
{
    QFileInfo info(path);
#if defined(Q_OS_WIN)
    QStringList args;
    if (!info.isDir())
        args << "/select,";
    args << QDir::toNativeSeparators(path);
    if (QProcess::startDetached("explorer", args))
        return;
#elif defined(Q_OS_MAC)
    QStringList args;
    args << "-e";
    args << "tell application \"Finder\"";
    args << "-e";
    args << "activate";
    args << "-e";
    args << "select POSIX file \"" + path + "\"";
    args << "-e";
    args << "end tell";
#if !defined(QT_DEBUG)
    args << "-e";
    args << "return";
#endif
    if (!QProcess::execute("/usr/bin/osascript", args))
        return;
#endif
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.isDir()? path : info.path()));
}

bool Util::warnIfNotWritable(const QString& filePath, QWidget* parent, const QString& caption)
{
    // Returns true if not writable.
    if (!filePath.isEmpty()) {
        // Do a hard check by writing to the file.
        QFile file(filePath);
        file.open(QIODevice::WriteOnly);
        if (file.write("") < 0) {
            QFileInfo fi(filePath);
            QMessageBox::warning(parent, caption,
                                 QObject::tr("Unable to write file %1\n"
                                    "Perhaps you do not have permission.\n"
                                    "Try again with a different folder.")
                                 .arg(fi.fileName()));
            return true;
        } else {
            file.remove();
        }
    }
    return false;
}

QString Util::producerTitle(const Mlt::Producer& producer)
{
    QString result;
    Mlt::Producer& p = const_cast<Mlt::Producer&>(producer);
    if (!p.is_valid() || p.is_blank()) return result;
    if (p.get(kShotcutTransitionProperty))
        return QObject::tr("Transition");
    if (p.get(kTrackNameProperty))
        return QObject::tr("Track: %1").arg(QString::fromUtf8(p.get(kTrackNameProperty)));
    if (tractor_type == p.type())
        return QObject::tr("Master");
    if (p.get(kShotcutCaptionProperty))
        return QString::fromUtf8(p.get(kShotcutCaptionProperty));
    if (p.get("resource"))
        return Util::baseName(QString::fromUtf8(p.get("resource")));
    return result;
}

QString Util::removeFileScheme(QUrl& url)
{
    QString path = url.url();
    if (url.scheme() == "file")
        path = url.url(QUrl::PreferLocalFile);
    return path;
}

QStringList Util::sortedFileList(const QList<QUrl>& urls)
{
    QStringList result;
    QMap<QString, QStringList> goproFiles;

    // First look for GoPro main files.
    foreach (QUrl url, urls) {
        QFileInfo fi(removeFileScheme(url));
        if (fi.baseName().size() == 8 && fi.suffix() == "MP4" && fi.baseName().startsWith("GOPR"))
            goproFiles[fi.baseName().mid(4)] << fi.filePath();
    }
    // Then, look for GoPro split files.
    foreach (QUrl url, urls) {
        QFileInfo fi(removeFileScheme(url));
        if (fi.baseName().size() == 8 && fi.suffix() == "MP4" && fi.baseName().startsWith("GP")) {
            QString goproNumber = fi.baseName().mid(4);
            // Only if there is a matching main GoPro file.
            if (goproFiles.contains(goproNumber) && goproFiles[goproNumber].size())
                goproFiles[goproNumber] << fi.filePath();
        }
    }
    // Next, sort the GoPro files.
    foreach (QString goproNumber, goproFiles.keys())
        goproFiles[goproNumber].sort(Qt::CaseSensitive);
    // Finally, build the list of all files.
    // Add all the GoPro files first.
    foreach (QStringList paths, goproFiles)
        result << paths;
    // Add all the non-GoPro files.
    foreach (QUrl url, urls) {
        QFileInfo fi(removeFileScheme(url));
        if (fi.baseName().size() == 8 && fi.suffix() == "MP4" &&
                (fi.baseName().startsWith("GOPR") || fi.baseName().startsWith("GP"))) {
            QString goproNumber = fi.baseName().mid(4);
            if (goproFiles.contains(goproNumber) && goproFiles[goproNumber].contains(fi.filePath()))
                continue;
        }
        result << fi.filePath();
    }
    return result;
}

int Util::coerceMultiple(int value, int multiple)
{
    return (value + multiple - 1) / multiple * multiple;
}

QList<QUrl> Util::expandDirectories(const QList<QUrl>& urls)
{
    QList<QUrl> result;
    foreach (QUrl url, urls) {
        QString path = Util::removeFileScheme(url);
        QFileInfo fi(path);
        if (fi.isDir()) {
            QDir dir(path);
            foreach (QFileInfo fi, dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name))
                result << fi.filePath();
        } else {
            result << url;
        }
    }
    return result;
}

uint Util::versionStringToUInt(const QString& s)
{
    // This only accepts up to 3 period-delimited fields where each
    // field value <= 999.
    uint version = 0;
    QStringList list = s.split('.').mid(0, 3);
    QList<QString>::const_reverse_iterator i;
    uint factor = 1;
    for (i = list.crbegin(); i != list.crend(); ++i, factor *= 1000)
         version += qMin(i->toUInt(), 999U) * factor;
    return version;
}
