/*
 * Copyright (c) 2020 Meltytech, LLC
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

#include "shotcut_mlt_properties.h"
#include "blipproducerwidget.h"
#include "ui_blipproducerwidget.h"
#include "util.h"
#include <MltProfile.h>

BlipProducerWidget::BlipProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BlipProducerWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->nameLabel);
    ui->preset->saveDefaultPreset(getPreset());
    ui->preset->loadPresets();
    on_periodSpinBox_valueChanged(ui->periodSpinBox->value());
}

BlipProducerWidget::~BlipProducerWidget()
{
    delete ui;
}

Mlt::Producer *BlipProducerWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile, "blipflash:");
    p->set("period", ui->periodSpinBox->value());
    p->set("force_seekable", 1);
    p->set(kShotcutCaptionProperty, ui->nameLabel->text().toUtf8().constData());
    p->set(kShotcutDetailProperty, detail().toUtf8().constData());
    return p;
}

Mlt::Properties BlipProducerWidget::getPreset() const
{
    Mlt::Properties p;
    p.set("period", ui->periodSpinBox->value());
    return p;
}

void BlipProducerWidget::loadPreset(Mlt::Properties &p)
{
    ui->periodSpinBox->setValue(p.get_int("period"));
    p.set(kShotcutDetailProperty, detail().toUtf8().constData());
}

void BlipProducerWidget::on_periodSpinBox_valueChanged(int value)
{
    ui->periodSpinBox->setSuffix(tr(" second(s)", nullptr, value));
    if (m_producer) {
        m_producer->set("period", value);
        m_producer->set(kShotcutDetailProperty, detail().toUtf8().constData());
        emit producerChanged(producer());
    }
}

void BlipProducerWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void BlipProducerWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}

QString BlipProducerWidget::detail() const
{
    return tr("Period: %1s").arg(ui->periodSpinBox->value());
}
