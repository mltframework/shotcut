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

#include "cropfilter.h"
#include "ui_cropfilter.h"
#include "mltcontroller.h"

CropFilter::CropFilter(Mlt::Filter filter, bool setDefaults, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CropFilter),
    m_filter(filter)
{
    ui->setupUi(this);
    if (setDefaults) {
        ui->centerCheckBox->setChecked(false);
        m_filter.set("center", 0);
        m_filter.set("center_bias", 0);
        m_filter.set("top", 0);
        m_filter.set("bottom", 0);
        m_filter.set("left", 0);
        m_filter.set("right", 0);
//        m_filter.set("use_profile", 1);
        ui->preset->saveDefaultPreset(m_filter);
    } else {
        ui->centerCheckBox->setChecked(m_filter.get_int("center"));
        ui->biasSpinner->setValue(m_filter.get_int("center_bias"));
        ui->topSpinner->setValue(m_filter.get_int("top"));
        ui->bottomSpinner->setValue(m_filter.get_int("bottom"));
        ui->leftSpinner->setValue(m_filter.get_int("left"));
        ui->rightSpinner->setValue(m_filter.get_int("right"));
    }
    ui->preset->loadPresets();
    ui->biasSlider->setMinimum(qMax(MLT.profile().height(), MLT.profile().width()) / -2);
    ui->biasSpinner->setMinimum(qMax(MLT.profile().height(), MLT.profile().width()) / -2);
    ui->biasSlider->setMaximum(qMax(MLT.profile().height(), MLT.profile().width()) / 2);
    ui->biasSpinner->setMaximum(qMax(MLT.profile().height(), MLT.profile().width()) / 2);
    ui->topSlider->setMaximum(MLT.profile().height());
    ui->topSpinner->setMaximum(MLT.profile().height());
    ui->bottomSlider->setMaximum(MLT.profile().height());
    ui->bottomSpinner->setMaximum(MLT.profile().height());
    ui->leftSlider->setMaximum(MLT.profile().width());
    ui->leftSpinner->setMaximum(MLT.profile().width());
    ui->rightSlider->setMaximum(MLT.profile().width());
    ui->rightSpinner->setMaximum(MLT.profile().width());
}

CropFilter::~CropFilter()
{
    delete ui;
}

void CropFilter::on_preset_selected(void* o)
{
    Mlt::Properties p(((Mlt::Properties*) o)->get_properties());
    if (p.get("center"))
        ui->centerCheckBox->setChecked(p.get_int("center"));
    if (p.get("center_bias"))
        ui->biasSpinner->setValue(p.get_int("center_bias"));
    if (p.get("top"))
        ui->topSpinner->setValue(p.get_int("top"));
    if (p.get("bottom"))
        ui->bottomSpinner->setValue(p.get_int("bottom"));
    if (p.get("left"))
        ui->leftSpinner->setValue(p.get_int("left"));
    if (p.get("right"))
        ui->rightSpinner->setValue(p.get_int("right"));
}

void CropFilter::on_preset_saveClicked()
{
    ui->preset->savePreset(&m_filter);
}

void CropFilter::on_centerCheckBox_toggled(bool checked)
{
    m_filter.set("center", (checked? 1 : 0));
    ui->biasSlider->setEnabled(checked);
    ui->biasSpinner->setEnabled(checked);
    ui->topSlider->setDisabled(checked);
    ui->topSpinner->setDisabled(checked);
    ui->bottomSlider->setDisabled(checked);
    ui->bottomSpinner->setDisabled(checked);
    ui->leftSlider->setDisabled(checked);
    ui->leftSpinner->setDisabled(checked);
    ui->rightSlider->setDisabled(checked);
    ui->rightSpinner->setDisabled(checked);
    MLT.refreshConsumer();
}

void CropFilter::on_biasSpinner_valueChanged(int arg1)
{
    m_filter.set("center_bias", arg1);
    MLT.refreshConsumer();
}

void CropFilter::on_topSpinner_valueChanged(int arg1)
{
    m_filter.set("top", arg1);
    MLT.refreshConsumer();
}

void CropFilter::on_bottomSpinner_valueChanged(int arg1)
{
    m_filter.set("bottom", arg1);
    MLT.refreshConsumer();
}

void CropFilter::on_leftSpinner_valueChanged(int arg1)
{
    m_filter.set("left", arg1);
    MLT.refreshConsumer();
}

void CropFilter::on_rightSpinner_valueChanged(int arg1)
{
    m_filter.set("right", arg1);
    MLT.refreshConsumer();
}
