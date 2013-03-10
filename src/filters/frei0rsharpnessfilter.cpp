/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * GL shader based on BSD licensed code from Peter Bengtsson:
 * http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
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

#include "frei0rsharpnessfilter.h"
#include "ui_frei0rsharpnessfilter.h"
#include "mltcontroller.h"

Frei0rSharpnessFilter::Frei0rSharpnessFilter(Mlt::Filter filter, bool setDefaults, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Frei0rSharpnessFilter),
    m_filter(filter),
    m_defaultAmount(0.5),
    m_defaultSize(0.5)
{
    ui->setupUi(this);
    if (setDefaults) {
        ui->amountSpinner->setValue(m_defaultAmount * 100.0);
        ui->sizeSpinner->setValue(m_defaultSize * 100.0);
    } else {
        ui->amountSpinner->setValue(m_filter.get_double("Amount") * 100.0);
        ui->sizeSpinner->setValue(m_filter.get_double("Size") * 100.0);
    }
}

Frei0rSharpnessFilter::~Frei0rSharpnessFilter()
{
    delete ui;
}

void Frei0rSharpnessFilter::on_amountSpinner_valueChanged(double arg1)
{
    ui->amountSlider->setValue(10 * arg1);
    m_filter.set("Amount", arg1 / 100.0);
    MLT.refreshConsumer();
}

void Frei0rSharpnessFilter::on_sizeSpinner_valueChanged(double arg1)
{
    ui->sizeSlider->setValue(10 * arg1);
    m_filter.set("Size", arg1 / 100.0);
    MLT.refreshConsumer();
}

void Frei0rSharpnessFilter::on_defaultAmountButton_clicked()
{
    ui->amountSpinner->setValue(m_defaultAmount * 100.0);
}

void Frei0rSharpnessFilter::on_defaultSizeButton_clicked()
{
    ui->sizeSpinner->setValue(m_defaultSize * 100.0);
}

void Frei0rSharpnessFilter::on_amountSlider_valueChanged(int value)
{
    ui->amountSpinner->setValue(double(value) / 10.0);
}

void Frei0rSharpnessFilter::on_sizeSlider_valueChanged(int value)
{
    ui->sizeSpinner->setValue(double(value) / 10.0);
}
