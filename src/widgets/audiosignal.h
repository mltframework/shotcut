/***************************************************************************
 *   Copyright (C) 2010 by Marco Gittler (g.marco@freenet.de)              *
 *   Copyright (C) 2012 by Dan Dennedy (dan@dennedy.org)                   *
 *   Copyright (C) 2015 Meltytech, LLC                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#ifndef AUDIOSIGNAL_H
#define AUDIOSIGNAL_H

#include <QWidget>
#include <QVector>
#include <QColor>
#include <stdint.h>

class QLabel;

class AudioSignal : public QWidget
{
    Q_OBJECT
public:
    AudioSignal(QWidget *parent = 0);

private:
    double valueToPixel(double in);
    QLabel* label;
    QVector<double> channels, peeks, peekage;
    QVector<int> dbscale;

protected:
    void paintEvent(QPaintEvent*);

public slots:
    void showAudio(const QVector<double>& dbLevels);
};

#endif
