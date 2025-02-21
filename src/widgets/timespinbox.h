/*
 * Copyright (c) 2012-2022 Meltytech, LLC
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

#ifndef TIMESPINBOX_H
#define TIMESPINBOX_H

#include <QLineEdit>
#include <QSpinBox>

class QRegularExpressionValidator;

class TimeSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    explicit TimeSpinBox(QWidget *parent = 0);

protected:
    QValidator::State validate(QString &input, int &pos) const;
    int valueFromText(const QString &text) const;
    QString textFromValue(int val) const;
    void keyPressEvent(QKeyEvent *event);

signals:
    void accepted();

private:
    QRegularExpressionValidator *m_validator;
};

class TimeSpinBoxLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit TimeSpinBoxLineEdit(QWidget *parent = 0);

protected:
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    bool m_selectOnMousePress;
};

#endif // TIMESPINBOX_H
