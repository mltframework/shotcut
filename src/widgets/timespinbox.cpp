/*
 * Copyright (c) 2012-2024 Meltytech, LLC
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

#include "timespinbox.h"

#include "mltcontroller.h"
#include "settings.h"

#include <QFontDatabase>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QRegularExpressionValidator>

TimeSpinBox::TimeSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    setLineEdit(new TimeSpinBoxLineEdit);
    setRange(0, INT_MAX);
    setAlignment(Qt::AlignRight);
    m_validator = new QRegularExpressionValidator(QRegularExpression(
                                                      "^\\s*(\\d*:){0,2}(\\d*[.;:])?\\d*\\s*$"),
                                                  this);
    setValue(0);
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(QGuiApplication::font().pointSize());
    setFont(font);
    setFixedWidth(fontMetrics().boundingRect("_HHH:MM:SS;FFF_").width());

    connect(&Settings, &ShotcutSettings::timeFormatChanged, this, [&]() { setValue(value()); });
}

QValidator::State TimeSpinBox::validate(QString &input, int &pos) const
{
    return m_validator->validate(input, pos);
}

int TimeSpinBox::valueFromText(const QString &text) const
{
    if (MLT.producer() && MLT.producer()->is_valid()) {
        return MLT.producer()->time_to_frames(text.toLatin1().constData());
    } else {
        return Mlt::Producer(MLT.profile(), "color", "").time_to_frames(text.toLatin1().constData());
    }
    return 0;
}

QString TimeSpinBox::textFromValue(int val) const
{
    if (MLT.producer() && MLT.producer()->is_valid()) {
        return MLT.producer()->frames_to_time(val, Settings.timeFormat());
    } else {
        return Mlt::Producer(MLT.profile(), "color", "").frames_to_time(val, Settings.timeFormat());
    }
    return QString();
}

void TimeSpinBox::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_PageUp || event->key() == Qt::Key_PageDown) {
        // Disable page up & page down (step by 10) since those keys are used for other things in Shotcut.
        event->ignore();
        return;
    }
    QSpinBox::keyPressEvent(event);
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        event->accept();
        emit accepted();
    }
}

TimeSpinBoxLineEdit::TimeSpinBoxLineEdit(QWidget *parent)
    : QLineEdit(parent)
    , m_selectOnMousePress(false)
{}

void TimeSpinBoxLineEdit::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);
    selectAll();
    m_selectOnMousePress = true;
}

void TimeSpinBoxLineEdit::focusOutEvent(QFocusEvent *event)
{
    // QLineEdit::focusOutEvent() calls deselect() on OtherFocusReason,
    // which prevents using the clipboard actions with the text.
    if (event->reason() != Qt::OtherFocusReason)
        QLineEdit::focusOutEvent(event);
}

void TimeSpinBoxLineEdit::mousePressEvent(QMouseEvent *event)
{
    QLineEdit::mousePressEvent(event);
    if (m_selectOnMousePress) {
        selectAll();
        m_selectOnMousePress = false;
    }
}
