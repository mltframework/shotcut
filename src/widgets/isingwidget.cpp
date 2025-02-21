/*
 * Copyright (c) 2012-2018 Meltytech, LLC
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

#include "isingwidget.h"
#include "ui_isingwidget.h"

#include "shotcut_mlt_properties.h"
#include "util.h"

static const char *kParamTemperature = "0";
static const char *kParamBorderGrowth = "1";
static const char *kParamSpontaneous = "2";

IsingWidget::IsingWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::IsingWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->nameLabel);
    ui->preset->saveDefaultPreset(getPreset());
    ui->preset->loadPresets();
}

IsingWidget::~IsingWidget()
{
    delete ui;
}

void IsingWidget::on_tempDial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set(kParamTemperature, value / 100.0);
        emit producerChanged(m_producer.data());
    }
    ui->tempSpinner->setValue(value / 100.0);
}

void IsingWidget::on_tempSpinner_valueChanged(double value)
{
    ui->tempDial->setValue(value * 100);
}

void IsingWidget::on_borderGrowthDial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set(kParamBorderGrowth, value / 100.0);
        emit producerChanged(m_producer.data());
    }
    ui->borderGrowthSpinner->setValue(value / 100.0);
}

void IsingWidget::on_borderGrowthSpinner_valueChanged(double value)
{
    ui->borderGrowthDial->setValue(value * 100);
}

void IsingWidget::on_spontGrowthDial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set(kParamSpontaneous, value / 100.0);
        emit producerChanged(producer());
    }
    ui->spontGrowthSpinner->setValue(value / 100.0);
}

void IsingWidget::on_spontGrowthSpinner_valueChanged(double value)
{
    ui->spontGrowthDial->setValue(value * 100);
}

Mlt::Producer *IsingWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile, "frei0r.ising0r");
    p->set(kParamTemperature, ui->tempSpinner->text().toLatin1().constData());
    p->set(kParamBorderGrowth, ui->borderGrowthSpinner->text().toLatin1().constData());
    p->set(kParamSpontaneous, ui->spontGrowthSpinner->text().toLatin1().constData());
    p->set(kShotcutCaptionProperty, ui->nameLabel->text().toUtf8().constData());
    p->set(kShotcutDetailProperty, ui->nameLabel->text().toUtf8().constData());
    return p;
}

Mlt::Properties IsingWidget::getPreset() const
{
    Mlt::Properties p;
    p.set(kParamTemperature, ui->tempSpinner->text().toLatin1().constData());
    p.set(kParamBorderGrowth, ui->borderGrowthSpinner->text().toLatin1().constData());
    p.set(kParamSpontaneous, ui->spontGrowthSpinner->text().toLatin1().constData());
    return p;
}

void IsingWidget::loadPreset(Mlt::Properties &p)
{
    ui->tempSpinner->setValue(p.get_double(kParamTemperature));
    ui->borderGrowthSpinner->setValue(p.get_double(kParamBorderGrowth));
    ui->spontGrowthSpinner->setValue(p.get_double(kParamSpontaneous));
}

void IsingWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void IsingWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}
