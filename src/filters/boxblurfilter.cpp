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

#include "boxblurfilter.h"
#include "ui_boxblurfilter.h"
#include "mltcontroller.h"

BoxblurFilter::BoxblurFilter(Mlt::Filter filter, bool setDefaults, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BoxblurFilter),
    m_filter(filter),
    m_defaultWidth(2),
    m_defaultHeight(2)
{
    m_filter.set("start", 1);
    ui->setupUi(this);
    if (setDefaults) {
        ui->widthSpinBox->setValue(m_defaultWidth);
        ui->heightSpinBox->setValue(m_defaultHeight);
    } else {
        ui->widthSpinBox->setValue(filter.get_int("hori"));
        ui->heightSpinBox->setValue(filter.get_int("vert"));
    }
}

BoxblurFilter::~BoxblurFilter()
{
    delete ui;
}

void BoxblurFilter::on_widthSpinBox_valueChanged(int arg1)
{
    m_filter.set("hori", arg1);
    MLT.refreshConsumer();
}

void BoxblurFilter::on_heightSpinBox_valueChanged(int arg1)
{
    m_filter.set("vert", arg1);
    MLT.refreshConsumer();
}

void BoxblurFilter::on_defaultWidthButton_clicked()
{
    ui->widthSpinBox->setValue(m_defaultWidth);
}

void BoxblurFilter::on_defaultHeightButton_clicked()
{
    ui->heightSpinBox->setValue(m_defaultHeight);
}
