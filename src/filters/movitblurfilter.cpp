/*
 * Copyright (c) 2013 Meltytech, LLC
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

#include "movitblurfilter.h"
#include "ui_movitblurfilter.h"
#include "mltcontroller.h"

MovitBlurFilter::MovitBlurFilter(Mlt::Filter filter, bool setDefaults, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MovitBlurFilter),
    m_filter(filter)
{
    ui->setupUi(this);
    Mlt::Filter f(MLT.profile(), "movit.blur");
    m_radiusDefault = f.get_double("radius");
    if (setDefaults)
        ui->doubleSpinBox->setValue(m_radiusDefault);
    else
        ui->doubleSpinBox->setValue(m_filter.get_double("radius"));
}

MovitBlurFilter::~MovitBlurFilter()
{
    delete ui;
}

void MovitBlurFilter::on_doubleSpinBox_valueChanged(double arg1)
{
    m_filter.set("radius", arg1);
    MLT.refreshConsumer();
}

void MovitBlurFilter::on_pushButton_clicked()
{
    ui->doubleSpinBox->setValue(m_radiusDefault);
}
