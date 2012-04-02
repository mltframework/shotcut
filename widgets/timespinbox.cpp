/*
 * Copyright (c) 2012 Meltytech, LLC
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

#include "timespinbox.h"
#include "mltcontroller.h"
#include <QtGui/QRegExpValidator>

TimeSpinBox::TimeSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    setRange(0, INT_MAX);
    setValue(0);
    setFixedWidth(this->fontMetrics().width("HHH:MM:SS.MMM"));
    setAlignment(Qt::AlignRight);
    m_validator = new QRegExpValidator(QRegExp("^\\s*(\\d*:){0,3}[.,]*\\d*\\s*$"), this);
}

QValidator::State TimeSpinBox::validate(QString &input, int &pos) const
{
    return m_validator->validate(input, pos);
}

int TimeSpinBox::valueFromText(const QString &text) const
{
    if (MLT.producer()) {
        MLT.producer()->set("_shotcut_position", text.toAscii().constData());
        return MLT.producer()->get_int("_shotcut_position");
    }
    return 0;
}

QString TimeSpinBox::textFromValue(int val) const
{
    if (MLT.producer()) {
        MLT.producer()->set("_shotcut_position", val);
        return MLT.producer()->get_time("_shotcut_position");
    }
    return QString();
}
