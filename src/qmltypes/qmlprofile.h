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

#ifndef PROFILE_H
#define PROFILE_H

#include <QObject>

class QmlProfile : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int width READ width CONSTANT)
    Q_PROPERTY(int height READ height CONSTANT)
    Q_PROPERTY(double aspectRatio READ aspectRatio CONSTANT)
    Q_PROPERTY(double fps READ fps CONSTANT)
    Q_PROPERTY(double sar READ sar CONSTANT)

public:
    static QmlProfile &singleton();

    int width() const;
    int height() const;
    double aspectRatio() const;
    double fps() const;
    double sar() const;

signals:
    void profileChanged();

private:
    explicit QmlProfile();
    QmlProfile(QmlProfile const &);
    void operator=(QmlProfile const &);
};

#endif // PROFILE_H
