/*
 * Copyright (c) 2015-2020 Meltytech, LLC
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
#include "toneproducerwidget.h"
#include "ui_toneproducerwidget.h"
#include "util.h"
#include <MltProfile.h>

ToneProducerWidget::ToneProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ToneProducerWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->nameLabel);
    ui->preset->saveDefaultPreset(getPreset());
    ui->preset->loadPresets();
}

ToneProducerWidget::~ToneProducerWidget()
{
    delete ui;
}

Mlt::Producer *ToneProducerWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile, "tone:");
    p->set("frequency", ui->frequencySpinBox->value());
    p->set("level", ui->levelSpinBox->value());
    p->set(kShotcutCaptionProperty, ui->nameLabel->text().toUtf8().constData());
    p->set(kShotcutDetailProperty, detail().toUtf8().constData());
    return p;
}

Mlt::Properties ToneProducerWidget::getPreset() const
{
    Mlt::Properties p;
    p.set("frequency", ui->frequencySpinBox->value());
    p.set("level", ui->levelSpinBox->value());
    return p;
}

void ToneProducerWidget::loadPreset(Mlt::Properties &p)
{
    ui->frequencySpinBox->setValue(p.get_int("frequency"));
    ui->levelSpinBox->setValue(p.get_int("level"));
    p.set(kShotcutDetailProperty, detail().toUtf8().constData());
}

void ToneProducerWidget::on_frequencySpinBox_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set("frequency", value);
        m_producer->set(kShotcutDetailProperty, detail().toUtf8().constData());
        emit modified();
    }
}

void ToneProducerWidget::on_levelSpinBox_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set("level", value);
        m_producer->set(kShotcutDetailProperty, detail().toUtf8().constData());
        emit modified();
    }
}

void ToneProducerWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void ToneProducerWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}

QString ToneProducerWidget::detail() const
{
    return tr("Tone: %1Hz %2dB").arg(ui->frequencySpinBox->value()).arg(
               ui->levelSpinBox->value());
}
