/*
 * Copyright (c) 2025 Meltytech, LLC
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

#include "qmlextension.h"

#include "Logger.h"
#include "qmltypes/qmlutilities.h"
#include "settings.h"

#include <QDir>
#include <QQmlComponent>

QmlExtensionFile::QmlExtensionFile(QObject *parent)
    : QObject(parent)
{}

QmlExtension *QmlExtension::load(const QString &id)
{
    QDir dir = QmlUtilities::qmlDir();
    dir.cd("extensions");
    QString fileName = id + ".qml";
    if (!dir.exists(fileName)) {
        LOG_ERROR() << fileName << "does not exist";
        return nullptr;
    }
    QString filePath = dir.absoluteFilePath(fileName);
    QQmlComponent component(QmlUtilities::sharedEngine(), filePath);
    QmlExtension *extension = qobject_cast<QmlExtension *>(component.create());
    if (!extension) {
        LOG_ERROR() << component.errorString();
    }
    return extension;
}

QmlExtension::QmlExtension(QObject *parent)
    : QObject(parent)
{}

void QmlExtension::setName(const QString &name)
{
    m_name = name;
    emit changed();
}

void QmlExtension::setVersion(const QString &version)
{
    m_version = version;
    emit changed();
}

QString QmlExtension::localPath(int index)
{
    if (index < 0 || index >= fileCount()) {
        LOG_ERROR() << "Invalid Index" << index;
        return QString();
    }
    QDir localPath = Settings.appDataLocation();
    localPath.mkdir("extensions");
    localPath.cd("extensions");
    localPath.mkdir(m_name);
    localPath.cd(m_name);
    return localPath.absoluteFilePath(m_files[index]->file());
}

bool QmlExtension::downloaded(int index)
{
    return QFile(localPath(index)).exists();
}
