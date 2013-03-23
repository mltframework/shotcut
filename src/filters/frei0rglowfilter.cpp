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

#include "frei0rglowfilter.h"
#include "ui_frei0rglowfilter.h"
#include "mltcontroller.h"

static const char* kParamBlur = "0";

Frei0rGlowFilter::Frei0rGlowFilter(Mlt::Filter filter, bool setDefaults, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Frei0rGlowFilter),
    m_filter(filter),
    m_defaultBlur(50)
{
    ui->setupUi(this);
    if (setDefaults) {
        ui->spinBox->setValue(m_defaultBlur);
    } else {
        ui->spinBox->setValue(100 * m_filter.get_double(kParamBlur));
    }
}

Frei0rGlowFilter::~Frei0rGlowFilter()
{
    delete ui;
}

void Frei0rGlowFilter::on_spinBox_valueChanged(int arg1)
{
    m_filter.set(kParamBlur, double(arg1) / 100.0);
    MLT.refreshConsumer();
}

void Frei0rGlowFilter::on_pushButton_clicked()
{
    ui->spinBox->setValue(m_defaultBlur);
}
