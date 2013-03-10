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

#include "movitglowfilter.h"
#include "ui_movitglowfilter.h"
#include "mltcontroller.h"

MovitGlowFilter::MovitGlowFilter(Mlt::Filter filter, bool setDefaults, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MovitGlowFilter),
    m_filter(filter)
{
    ui->setupUi(this);
    Mlt::Filter f(MLT.profile(), "movit.glow");
    m_radiusDefault = f.get_double("radius");
    m_blurMixDefault = f.get_double("blur_mix");
    m_cutoffDefault = f.get_double("highlight_cutoff");
    if (setDefaults) {
        ui->radiusSpinBox->setValue(m_radiusDefault);
        ui->blurMixSpinBox->setValue(m_blurMixDefault);
        ui->highlightCutoffSpinBox->setValue(m_cutoffDefault);
        ui->preset->saveDefaultPreset(m_filter);
    } else {
        ui->radiusSpinBox->setValue(m_filter.get_double("radius"));
        ui->blurMixSpinBox->setValue(m_filter.get_double("blur_mix"));
        ui->highlightCutoffSpinBox->setValue(m_filter.get_double("highlight_cutoff"));
    }
    ui->preset->loadPresets();
}

MovitGlowFilter::~MovitGlowFilter()
{
    delete ui;
}

void MovitGlowFilter::on_radiusSpinBox_valueChanged(double arg1)
{
    m_filter.set("radius", arg1);
    MLT.refreshConsumer();
}

void MovitGlowFilter::on_blurMixSpinBox_valueChanged(double arg1)
{
    m_filter.set("blur_mix", arg1);
    MLT.refreshConsumer();
}

void MovitGlowFilter::on_highlightCutoffSpinBox_valueChanged(double arg1)
{
    m_filter.set("highlight_cutoff", arg1);
    MLT.refreshConsumer();
}

void MovitGlowFilter::on_radiusDefaultButton_clicked()
{
    ui->radiusSpinBox->setValue(m_radiusDefault);
}

void MovitGlowFilter::on_blurMixDefaultButton_clicked()
{
    ui->blurMixSpinBox->setValue(m_blurMixDefault);
}

void MovitGlowFilter::on_cutoffDefaultButton_clicked()
{
    ui->highlightCutoffSpinBox->setValue(m_cutoffDefault);
}

void MovitGlowFilter::on_preset_selected(void* p)
{
    Mlt::Properties* properties = (Mlt::Properties*) p;
    if (properties->get("radius"))
        ui->radiusSpinBox->setValue(properties->get_double("radius"));
    if (properties->get("blur_mix"))
        ui->blurMixSpinBox->setValue(properties->get_double("blur_mix"));
    if (properties->get("highlight_cutoff"))
        ui->highlightCutoffSpinBox->setValue(properties->get_double("highlight_cutoff"));
    delete properties;
}

void MovitGlowFilter::on_preset_saveClicked()
{
    ui->preset->savePreset(&m_filter);
}
