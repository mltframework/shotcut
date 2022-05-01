/*
 * Copyright (c) 2011-2015 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 * Loosely based on ideas from KAutoSaveFile by Jacob R Rideout <kde@jacobrideout.net>
 * and Kdenlive by Jean-Baptiste Mardelle.
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

#ifndef AUTOSAVEFILE_H
#define AUTOSAVEFILE_H

#include <QtCore/QFile>
#include <QtCore/QString>

class AutoSaveFile : public QFile
{
    Q_OBJECT
public:
    explicit AutoSaveFile(const QString &filename, QObject *parent = 0);
    ~AutoSaveFile();

    QString managedFileName() const
    {
        return m_managedFile;
    }
    void changeManagedFile(const QString &filename);

    virtual bool open(OpenMode openmode);
    static AutoSaveFile *getFile(const QString &filename);
    static QString path();

private:
    Q_DISABLE_COPY(AutoSaveFile)
    QString m_managedFile;
    bool m_managedFileNameChanged;
};

#endif // AUTOSAVEFILE_H
