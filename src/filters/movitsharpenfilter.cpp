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

#include "movitsharpenfilter.h"
#include "ui_movitsharpenfilter.h"
#include "mltcontroller.h"

MovitSharpenFilter::MovitSharpenFilter(Mlt::Filter filter, bool setDefaults, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MovitSharpenFilter),
    m_filter(filter)
{
    ui->setupUi(this);
    ui->label->hide();
    ui->matrixSizeSpinner->hide();
    ui->defaultMatrixSizeButton->hide();
    Mlt::Filter f(MLT.profile(), "movit.sharpen");
    m_defaultMatrixSize = f.get_int("matrix_size");
    m_defaultCircleRadius = f.get_double("circle_radius");
    m_defaultGaussianRadius = f.get_double("gaussian_radius");
    m_defaultCorrelation = f.get_double("correlation");
    m_defaultNoiseLevel = f.get_double("noise");
    if (setDefaults) {
//        ui->matrixSizeSpinner->setValue(m_defaultMatrixSize);
        ui->circleRadiusSpinner->setValue(m_defaultCircleRadius);
        ui->gaussianRadiusSpinner->setValue(m_defaultGaussianRadius);
        ui->correlationSpinner->setValue(m_defaultCorrelation);
        ui->noiseLevelSpinner->setValue(m_defaultNoiseLevel);
        ui->preset->saveDefaultPreset(filter);
    } else {
//        ui->matrixSizeSpinner->setValue(filter.get_int("matrix_size"));
        ui->circleRadiusSpinner->setValue(filter.get_double("circle_radius"));
        ui->gaussianRadiusSpinner->setValue(filter.get_double("gaussian_radius"));
        ui->correlationSpinner->setValue(filter.get_double("correlation"));
        ui->noiseLevelSpinner->setValue(filter.get_double("noise"));
    }
    ui->preset->loadPresets();
}

MovitSharpenFilter::~MovitSharpenFilter()
{
    delete ui;
}

void MovitSharpenFilter::on_preset_selected(void* p)
{
    Mlt::Properties* properties = (Mlt::Properties*) p;
//    if (properties->get("matrix_size"))
//        ui->matrixSizeSpinner->setValue(properties->get_int("matrix_size"));
    if (properties->get("circle_radius"))
        ui->circleRadiusSpinner->setValue(properties->get_double("circle_radius"));
    if (properties->get("gaussian_radius"))
        ui->gaussianRadiusSpinner->setValue(properties->get_double("gaussian_radius"));
    if (properties->get("correlation"))
        ui->correlationSpinner->setValue(properties->get_double("correlation"));
    if (properties->get("noise"))
        ui->noiseLevelSpinner->setValue(properties->get_double("noise"));
    delete properties;
}

void MovitSharpenFilter::on_preset_saveClicked()
{
    ui->preset->savePreset(&m_filter);
}

void MovitSharpenFilter::on_matrixSizeSpinner_valueChanged(int arg1)
{
//    m_filter.set("matrix_size", arg1);
//    MLT.refreshConsumer();
}

void MovitSharpenFilter::on_circleRadiusSpinner_valueChanged(double arg1)
{
    m_filter.set("circle_radius", arg1);
    MLT.refreshConsumer();
}

void MovitSharpenFilter::on_gaussianRadiusSpinner_valueChanged(double arg1)
{
    m_filter.set("gaussian_radius", arg1);
    MLT.refreshConsumer();
}

void MovitSharpenFilter::on_correlationSpinner_valueChanged(double arg1)
{
    ui->correlationSlider->setValue(arg1 * 1000.0);
    m_filter.set("correlation", arg1);
    MLT.refreshConsumer();
}

void MovitSharpenFilter::on_correlationSlider_valueChanged(int value)
{
    ui->correlationSpinner->setValue(double(value) / 1000.0);
}

void MovitSharpenFilter::on_noiseLevelSpinner_valueChanged(double arg1)
{
    ui->noiseLevelSlider->setValue(arg1 * 1000.0);
    m_filter.set("noise", arg1);
    MLT.refreshConsumer();
}

void MovitSharpenFilter::on_noiseLevelSlider_valueChanged(int value)
{
    ui->noiseLevelSpinner->setValue(double(value) / 1000.0);
}

void MovitSharpenFilter::on_defaultMatrixSizeButton_clicked()
{
    ui->matrixSizeSpinner->setValue(m_defaultMatrixSize);
}

void MovitSharpenFilter::on_defaultCircleRadiusButton_clicked()
{
    ui->circleRadiusSpinner->setValue(m_defaultCircleRadius);
}

void MovitSharpenFilter::on_defaultGaussianRadiusButton_clicked()
{
    ui->gaussianRadiusSpinner->setValue(m_defaultGaussianRadius);
}

void MovitSharpenFilter::on_defaultCorrelationButton_clicked()
{
    ui->correlationSpinner->setValue(m_defaultCorrelation);
}

void MovitSharpenFilter::on_defaultNoiseLevelButton_clicked()
{
    ui->noiseLevelSpinner->setValue(m_defaultNoiseLevel);
}
