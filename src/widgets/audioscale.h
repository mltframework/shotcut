/*
 * Copyright (c) 2015-2026 Meltytech, LLC
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

#ifndef AUDIOSCALE_H
#define AUDIOSCALE_H

#include <QWidget>

class QMouseEvent;

class AudioScale : public QWidget
{
    Q_OBJECT
public:
    explicit AudioScale(QWidget *parent = 0);

signals:
    void volumeSelected(double gain);

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *event) override;

private:
    QVector<int> dbscale;
};

#endif // AUDIOSCALE_H
