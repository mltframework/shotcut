/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#ifndef QMLAPPLICATION_H
#define QMLAPPLICATION_H

#include <QObject>
#include <QDir>
#include <QPoint>
#include <QColor>

class QmlApplication : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Qt::WindowModality dialogModality READ dialogModality);
    Q_PROPERTY(QPoint mousePos READ mousePos);
    Q_PROPERTY(QColor toolTipBaseColor READ toolTipBaseColor NOTIFY paletteChanged)
    Q_PROPERTY(QColor toolTipTextColor READ toolTipTextColor NOTIFY paletteChanged)
    Q_PROPERTY(QString OS READ OS CONSTANT)
    Q_PROPERTY(QString numericLocale READ numericLocale CONSTANT)

public:
    static QmlApplication& singleton();
    static Qt::WindowModality dialogModality();
    static QPoint mousePos();
    static QColor toolTipBaseColor();
    static QColor toolTipTextColor();
    static QString OS();
    static QString numericLocale();

signals:
    void paletteChanged();

private:
    explicit QmlApplication();
    QmlApplication(QmlApplication const&);
    void operator=(QmlApplication const&);
};

#endif // QMLAPPLICATION_H
