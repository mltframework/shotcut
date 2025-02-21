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

#include "lissajouswidget.h"
#include "ui_lissajouswidget.h"

#include "shotcut_mlt_properties.h"
#include "util.h"

static const char *kParamRatioX = "0";
static const char *kParamRatioY = "1";

LissajousWidget::LissajousWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LissajousWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->nameLabel);
    ui->preset->saveDefaultPreset(getPreset());
    ui->preset->loadPresets();
}

LissajousWidget::~LissajousWidget()
{
    delete ui;
}

void LissajousWidget::on_xratioDial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set(kParamRatioX, value / 100.0);
        emit producerChanged(m_producer.data());
    }
    ui->xratioSpinner->setValue(value / 100.0);
}

void LissajousWidget::on_xratioSpinner_valueChanged(double value)
{
    ui->xratioDial->setValue(value * 100);
}

void LissajousWidget::on_yratioDial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set(kParamRatioY, value / 100.0);
        emit producerChanged(m_producer.data());
    }
    ui->yratioSpinner->setValue(value / 100.0);
}

void LissajousWidget::on_yratioSpinner_valueChanged(double value)
{
    ui->yratioDial->setValue(value * 100);
}

Mlt::Producer *LissajousWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile, "frei0r.lissajous0r");
    p->set(kParamRatioX, ui->xratioSpinner->text().toLatin1().constData());
    p->set(kParamRatioY, ui->yratioSpinner->text().toLatin1().constData());
    p->set(kShotcutCaptionProperty, ui->nameLabel->text().toUtf8().constData());
    p->set(kShotcutDetailProperty, ui->nameLabel->text().toUtf8().constData());
    return p;
}

Mlt::Properties LissajousWidget::getPreset() const
{
    Mlt::Properties p;
    p.set(kParamRatioX, ui->xratioSpinner->text().toLatin1().constData());
    p.set(kParamRatioY, ui->yratioSpinner->text().toLatin1().constData());
    return p;
}

void LissajousWidget::loadPreset(Mlt::Properties &p)
{
    ui->xratioSpinner->setValue(p.get_double(kParamRatioX));
    ui->yratioSpinner->setValue(p.get_double(kParamRatioY));
}

void LissajousWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void LissajousWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}
