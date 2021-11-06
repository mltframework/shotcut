/*
 * Copyright (c) 2021 Meltytech, LLC
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

#ifndef EDITMARKERWIDGET_H
#define EDITMARKERWIDGET_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class TimeSpinBox;

class EditMarkerWidget : public QWidget
{
    Q_OBJECT

public:
    EditMarkerWidget(QWidget *parent);
    EditMarkerWidget(QWidget *parent, const QString& text, const QColor& color, int start, int end, int maxEnd);
    virtual ~EditMarkerWidget();
    QString getText();
    QColor getColor();
    int getStart();
    int getEnd();
    void setValues(const QString& text, const QColor& color, int start, int end, int maxEnd);

signals:
    void valuesChanged();

private slots:
    void on_colorButton_clicked();
    void on_startSpinner_valueChanged(int);
    void on_endSpinner_valueChanged(int);

private:
    void updateDuration();

    QLineEdit* m_textField;
    QPushButton* m_colorButton;
    QLabel* m_colorLabel;
    TimeSpinBox* m_startSpinner;
    TimeSpinBox* m_endSpinner;
    QLabel* m_durationLabel;
};

#endif // EDITMARKERWIDGET_H
