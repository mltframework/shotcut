/*
 * Copyright (c) 2015 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#ifndef AUDIOMETERWIDGET_H
#define AUDIOMETERWIDGET_H

#include <QLinearGradient>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include <stdint.h>

class QLabel;

class AudioMeterWidget : public QWidget
{
    Q_OBJECT
public:
    AudioMeterWidget(QWidget *parent = 0);
    void setDbLabels(const QVector<int> &labels);
    void setChannelLabels(const QStringList &labels);
    void setChannelLabelUnits(const QString &units);
    void setOrientation(Qt::Orientation orientation);

public slots:
    void showAudio(const QVector<double> &dbLevels);

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *) Q_DECL_OVERRIDE;

private:
    void calcGraphRect();
    void drawDbLabels(QPainter &);
    void drawChanLabels(QPainter &);
    void drawBars(QPainter &);
    void drawPeaks(QPainter &);
    void updateToolTip();
    QRectF m_graphRect;
    QSizeF m_barSize;
    Qt::Orientation m_orient;
    QVector<double> m_levels;
    QVector<double> m_peaks;
    QVector<int> m_dbLabels;
    QStringList m_chanLabels;
    QLinearGradient m_gradient;
    double m_maxDb;
    QString m_chanLabelUnits;
};

#endif
