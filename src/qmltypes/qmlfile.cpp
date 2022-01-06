/*
 * Copyright (c) 2014-2022 Meltytech, LLC
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
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <Logger.h>

QmlFile::QmlFile(QObject* parent)
    : QObject(parent)
    , m_url()
{
}

QString QmlFile::getUrl()
{
    auto s = QUrl::fromPercentEncoding(m_url.toString().toUtf8());
#ifdef Q_OS_WIN
    if (s.size() > 2 && s[1]  == ':' && s[2]  == '/') {
        s[0] = s[0].toUpper();
    }
#endif
    return s;
}

void QmlFile::setUrl(QString text)
{
    QUrl url = text.replace('\\', "/");
    QUrl::FormattingOptions options =
            QUrl::RemoveScheme |
            QUrl::RemovePassword |
            QUrl::RemoveUserInfo |
            QUrl::RemovePort |
            QUrl::RemoveAuthority |
            QUrl::RemoveQuery;
#ifdef Q_OS_WIN
    // If the scheme is a drive letter, do not remove it.
    if (url.scheme().size() == 1) {
        options ^= QUrl::RemoveScheme;
    // QUrl removes the host from a UNC path when removing the scheme.
    } else if (text.startsWith("file://") && text.size() > 9 && text[9] != ':') {
        options ^= QUrl::RemoveScheme;
        options ^= QUrl::RemoveAuthority;
    }

    QUrl adj = url.adjusted(options);
    QString s = adj.toString();

    // If there is a slash before a drive letter.
    // On Windows, file URLs look like file:///C:/Users/....
    // The scheme is removed but only "://" (not 3 slashes) between scheme and path.
    if (s.size() > 2 && s[0] == '/' && s[2]  == ':') {
        // Remove the leading slash.
        adj = s.mid(1);
    } else if (s.startsWith("file://")) { // UNC path
        // Remove the scheme.
        adj = s.mid(5);
    }
#else
    QUrl adj = url.adjusted(options);
#endif

    if(m_url != adj) {
        m_url = adj;
        emit urlChanged(m_url);
    }
}

QString QmlFile::getFileName()
{
    return QFileInfo(m_url.toString()).fileName();
}

QString QmlFile::getPath()
{
    return QDir::toNativeSeparators(QFileInfo(getUrl()).path());
}

QString QmlFile::getFilePath()
{
    return QDir::toNativeSeparators(getUrl());
}

void QmlFile::copyFromFile(QString source)
{
    if (QFile::exists(m_url.toString()))
    {
        QFile::remove(m_url.toString());
    }

    QFile inFile(source);
    QFile outfile(m_url.toString());
    inFile.open(QFile::ReadOnly);
    outfile.open(QFile::WriteOnly);
    outfile.write(inFile.readAll());
    outfile.close();
}

bool QmlFile::exists()
{
    return QFileInfo(m_url.toString()).exists();
}

QString QmlFile::suffix()
{
    return QFileInfo(m_url.toString()).suffix();
}
