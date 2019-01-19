/*
 * Copyright (c) 2019 Meltytech, LLC
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

#include "webvfxtemplatesmodel.h"
#include "qmlutilities.h"
#include "mltcontroller.h"
#include <Logger.h>
#include <QDir>
#include <QFile>

WebvfxTemplatesModel::WebvfxTemplatesModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QDir dir = QmlUtilities::qmlDir();
    dir.cd("filters");
    dir.cd("webvfx");
    dir.cd("templates");
    dir.setFilter(QDir::Dirs | QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Executable);
    foreach (QString protocol, dir.entryList()) {
        QDir protocolDir = dir;
        protocolDir.cd(protocol);
        protocolDir.setFilter(QDir::Dirs | QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Executable);
        foreach (QString category, protocolDir.entryList()) {
            QDir categoryDir = protocolDir;
            categoryDir.cd(category);
            categoryDir.setFilter(QDir::Dirs | QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Executable);
            foreach (QString templateName, categoryDir.entryList()) {
                m_list << categoryDir.filePath(templateName);
            }
        }
    }
}

int WebvfxTemplatesModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_list.size();
}

QVariant WebvfxTemplatesModel::data(const QModelIndex& index, int role) const
{
    if (index.isValid() && index.row() < m_list.size()) {
        QDir dir(m_list[index.row()]);
        switch (role) {
        case Qt::DisplayRole:
            return dir.dirName();
        case ProtocolRole:
            if (dir.cdUp() && dir.cdUp())
                return dir.dirName();
            break;
        case CategoryRole:
            if (dir.cdUp())
                return dir.dirName();
            break;
        case PathRole:
            return dir.path();
        default:
            break;
        }
    }
    return QVariant();
}

QVariant WebvfxTemplatesModel::data(int row, int role) const
{
    return data(index(row), role);
}

QHash<int, QByteArray> WebvfxTemplatesModel::roleNames() const
{
     QHash<int, QByteArray> result = QAbstractListModel::roleNames();
     result[Qt::DisplayRole] = "name";
     result[PathRole] = "path";
     result[ProtocolRole] = "protocol";
     result[CategoryRole] = "category";
     return result;
}

bool WebvfxTemplatesModel::needsFolder(int row) const
{
    bool result = false;
    if (row < m_list.size()) {
        QDir dir(m_list[row]);
        dir.setFilter(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
        result = dir.count() > 1;
    }
    return result;
}

QString WebvfxTemplatesModel::copyTemplate(int row, QString path) const
{
    QString result;

    // Validate the request.
    if (row < m_list.size()) {
        QDir templateDir(m_list[row]);

        // Check if the destination is a folder.
        if (QFileInfo(path).isDir()) {
            QDir projectDir(path);

            // Copy all of the HTML and assets.
            foreach (QString fileName, templateDir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot)) {
                QFile::copy(templateDir.filePath(fileName), projectDir.filePath(fileName));
                // Return the name of the first HTML file found.
                if (result.isEmpty() && (fileName.endsWith(".html") || fileName.endsWith(".htm")))
                    result = projectDir.filePath(fileName);
            }
        } else {
            // Copy the HTML file.
            foreach (QString fileName, templateDir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot)) {
                if (fileName.endsWith(".html") || fileName.endsWith(".htm")) {
                    QFile::remove(path);
                    QFile::copy(templateDir.filePath(fileName), path);
                    // Return the name of the first HTML file found.
                    result = path;
                    break;
                }
            }
        }
    }
    return result;
}

QString WebvfxTemplatesModel::copyTemplate(int row) const
{
    QString result;
    QDir projectDir(MLT.projectFolder());

    // Validate the request.
    if (row < m_list.size() && projectDir.exists()) {
        QDir templateDir(m_list[row]);

        // Find a unique folder name in the project folder.
        for (unsigned i = 1; i < std::numeric_limits<unsigned>::max(); i++) {
            QString subdirName = QString::fromLatin1("%1 %2").arg(templateDir.dirName()).arg(i);
            if (!projectDir.exists(subdirName)) {
                // Make the HTML folder in the project folder.
                if (projectDir.mkdir(subdirName) && projectDir.cd(subdirName)) {
                    result = copyTemplate(row, projectDir.path());
                } else {
                    LOG_ERROR() << "Failed to create folder" << projectDir.filePath(subdirName);
                }
                break;
            }
        }
    }
    return result;
}
