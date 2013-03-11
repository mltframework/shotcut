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

#include "saturationfilter.h"
#include "ui_saturationfilter.h"
#include "mltcontroller.h"

SaturationFilter::SaturationFilter(Mlt::Filter filter, bool setDefaults, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SaturationFilter),
    m_filter(filter)
{
    m_isMovit = QString(m_filter.get("mlt_service")).startsWith("movit.");
    ui->setupUi(this);
    if (setDefaults) {
        ui->doubleSpinBox->setValue(100.0);
        if (!m_isMovit)
            m_filter.set("Saturation", 0.125);
    }
    else if (m_isMovit)
        ui->doubleSpinBox->setValue(m_filter.get_double("saturation") * 100.0);
    else
        ui->doubleSpinBox->setValue(m_filter.get_double("Saturation") * 800.0);
}

SaturationFilter::~SaturationFilter()
{
    delete ui;
}

void SaturationFilter::on_doubleSpinBox_valueChanged(double arg1)
{
    if (m_isMovit)
        m_filter.set("saturation", arg1 / 100.0);
    else
        m_filter.set("Saturation", arg1 / 800.0);
    ui->horizontalSlider->setValue(10 * arg1);
    MLT.refreshConsumer();
}

void SaturationFilter::on_horizontalSlider_valueChanged(int value)
{
    ui->doubleSpinBox->setValue(double(value) / 10.0);
}

void SaturationFilter::on_pushButton_clicked()
{
    ui->doubleSpinBox->setValue(100.0);
}
