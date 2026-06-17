/*
 * Copyright (c) 2014-2026 Meltytech, LLC
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

#include "qmlfile.h"

#include "Logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

/*!
    \qmltype File
    \inqmlmodule org.shotcut.qml
    \brief A file URL wrapper with optional filesystem change watching.

    Use \c File in filter panels to manage paths to auxiliary files
    (e.g. LUT files, preset data files). Set \l url and call \l{File::watch()}{watch()}
    to receive \l fileChanged notifications when the file is modified externally.

    \code
    File {
        id: lutFile
        url: filter.get("av.file")
        onFileChanged: filter.set("av.file", path)
    }
    \endcode
*/

/*!
    \qmlsignal File::urlChanged(url url)
    \brief Emitted when \l url changes. \a url is the new URL value.
*/

/*!
    \qmlsignal File::fileChanged(string path)
    \brief Emitted when the watched file is modified or deleted externally.
    \a path is the full file system path. Connect a file watcher by calling \l{File::watch()}{watch()} first.
*/

QmlFile::QmlFile(QObject *parent)
    : QObject(parent)
    , m_url()
{}

/*!
    \qmlproperty string File::url
    \brief The file URL as a string (e.g. \c "file:///home/user/lut.cube").
    Setting this property stops watching any previously watched file.
*/

QString QmlFile::getUrl()
{
    auto s = QUrl::fromPercentEncoding(m_url.toString().toUtf8());
#ifdef Q_OS_WIN
    if (s.size() > 2 && s[1] == ':' && s[2] == '/') {
        s[0] = s[0].toUpper();
    }
#endif
    return s;
}

void QmlFile::setUrl(QString text)
{
    QUrl url = text.replace('\\', "/");
    QString s = url.toString();
    QUrl::FormattingOptions options = QUrl::RemoveScheme | QUrl::RemovePassword
                                      | QUrl::RemoveUserInfo | QUrl::RemovePort
                                      | QUrl::RemoveAuthority | QUrl::RemoveQuery;

    if (s.startsWith("file://") && s.size() > 9 && s[9] != ':') {
        // QUrl removes the host from a UNC path when removing the scheme.
        options ^= QUrl::RemoveScheme;
        options ^= QUrl::RemoveAuthority;
    }

#ifdef Q_OS_WIN
    // If the scheme is a drive letter, do not remove it.
    if (url.scheme().size() == 1) {
        options ^= QUrl::RemoveScheme;
    }
    // RemoveAuthority removes the server from a UNC path.
    if (s.startsWith("//")) {
        options ^= QUrl::RemoveAuthority;
    }
#endif

    s = url.adjusted(options).toString();

#ifdef Q_OS_WIN
    // If there is a slash before a drive letter.
    // On Windows, file URLs look like file:///C:/Users/....
    // The scheme is removed but only "://" (not 3 slashes) between scheme and path.
    if (s.size() > 2 && s[0] == '/' && s[2] == ':') {
        // Remove the leading slash.
        s = s.mid(1);
    }
#endif

    if (s.startsWith("file://")) { // UNC path
        // Remove the scheme.
        s = s.mid(5);
    }

    if (s.startsWith("///")) {
        // Linux leaves 3 leading slashes sometimes
        s = s.mid(2);
    }

    QUrl adj = s;

    if (m_url != adj) {
        m_url = adj;
        emit urlChanged(m_url);
    }
}

/*!
    \qmlproperty string File::fileName
    \brief The base file name without the directory path (e.g. \c "lut.cube").
*/

QString QmlFile::getFileName()
{
    return QFileInfo(getUrl()).fileName();
}

/*!
    \qmlproperty string File::path
    \brief The directory path portion of the URL (without the file name).
*/

QString QmlFile::getPath()
{
    return QDir::toNativeSeparators(QFileInfo(getUrl()).path());
}

/*!
    \qmlproperty string File::filePath
    \brief The full local filesystem path (e.g. \c "/home/user/lut.cube").
*/

QString QmlFile::getFilePath()
{
    return QDir::toNativeSeparators(getUrl());
}

/*!
    \qmlmethod File::copyFromFile(string source)
    \brief Copies the file at \a source to the path described by \l url.
*/

void QmlFile::copyFromFile(QString source)
{
    if (QFile::exists(m_url.toString())) {
        QFile::remove(m_url.toString());
    }

    QFile inFile(source);
    QFile outfile(m_url.toString());
    if (!inFile.open(QFile::ReadOnly)) {
        LOG_ERROR() << "Failed to open source file for reading" << source;
        return;
    }

    if (!outfile.open(QFile::WriteOnly)) {
        LOG_ERROR() << "Failed to open destination file for writing" << m_url.toString();
        return;
    }
    outfile.write(inFile.readAll());
    outfile.close();
}

/*!
    \qmlmethod bool File::exists()
    \brief Returns \c true if the file at \l url exists on the filesystem.
*/

bool QmlFile::exists()
{
    return QFileInfo(m_url.toString()).exists();
}

/*!
    \qmlmethod string File::suffix()
    \brief Returns the file extension without the leading dot (e.g. \c "cube").
*/

QString QmlFile::suffix()
{
    return QFileInfo(m_url.toString()).suffix();
}

/*!
    \qmlmethod File::watch()
    \brief Begins watching the file at \l url for external changes.
    When the file is modified or deleted, \l fileChanged is emitted.
*/
void QmlFile::watch()
{
    m_watcher.reset(new QFileSystemWatcher({getUrl()}));
    connect(m_watcher.get(), &QFileSystemWatcher::fileChanged, this, &QmlFile::fileChanged);
}
