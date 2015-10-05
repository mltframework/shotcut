/*
 * Copyright (c) 2013-2015 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <QObject>
#include <QImage>
#include <QMutex>

class Database : public QObject
{
    Q_OBJECT
    explicit Database(QObject *parent = 0);

public:
    static Database& singleton(QWidget* parent = 0);
    ~Database();

    bool upgradeVersion1();
    bool putThumbnail(const QString& hash, const QImage& image);
    QImage getThumbnail(const QString& hash);

private:
    void deleteOldThumbnails();
    QMutex m_mutex;
};

#define DB Database::singleton()

#endif // DATABASE_H
