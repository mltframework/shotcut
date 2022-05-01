/*
 * Copyright (c) 2020 Meltytech, LLC
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

#ifndef FRAMERATEWIDGET_H
#define FRAMERATEWIDGET_H

#include <QWidget>

class QDoubleSpinBox;
class QComboBox;

class FrameRateWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FrameRateWidget(QWidget *parent = nullptr);
    double fps();

public slots:
    void setFps(double);

signals:
    void fpsChanged(double);

private slots:
    void on_fpsSpinner_editingFinished();
    void on_fpsComboBox_activated(const QString &arg1);

private:
    QDoubleSpinBox *m_fpsSpinner;
    QComboBox *m_fpsComboBox;
    double m_fps;
};

#endif // FRAMERATEWIDGET_H
