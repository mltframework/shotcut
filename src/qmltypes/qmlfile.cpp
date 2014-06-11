/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Brian Matherly <pez4brian@yahoo.com>
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

QmlFile::QmlFile(QObject* parent)
    : QObject(parent)
    , m_url()
{
}

QUrl QmlFile::getUrl()
{
    return m_url;
}

void QmlFile::setUrl(const QUrl& url)
{
    QUrl adj = url.adjusted(QUrl::RemoveScheme |
                            QUrl::RemovePassword |
                            QUrl::RemoveUserInfo |
                            QUrl::RemovePort |
                            QUrl::RemoveAuthority |
                            QUrl::RemoveQuery );
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
    return QFileInfo(m_url.toString()).path();
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
