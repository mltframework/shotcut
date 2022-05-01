/*
 * Copyright (c) 2013-2021 Meltytech, LLC
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

#ifndef DATABASE_H
#define DATABASE_H

#include <QImage>
#include <QDir>
#include <QTimer>

class Database : public QObject
{
    Q_OBJECT
    explicit Database(QObject *parent = 0);

public:
    static Database &singleton(QObject *parent = 0);

    bool putThumbnail(const QString &hash, const QImage &image);
    QImage getThumbnail(const QString &hash);

private:
    QDir thumbnailsDir();
    QTimer m_deleteTimer;

private slots:
    void deleteOldThumbnails();
};

#define DB Database::singleton()

#endif // DATABASE_H
