/*
 * Copyright (c) 2012-2022 Meltytech, LLC
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

#include "alsawidget.h"
#include "ui_alsawidget.h"

#include "mltcontroller.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

AlsaWidget::AlsaWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AlsaWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label_2);
    ui->applyButton->hide();
    ui->preset->saveDefaultPreset(getPreset());
    ui->preset->loadPresets();
    ui->lineEdit->setText(Settings.audioInput());
}

AlsaWidget::~AlsaWidget()
{
    delete ui;
}

Mlt::Producer *AlsaWidget::newProducer(Mlt::Profile &profile)
{
    QString s("alsa:%1");
    if (ui->lineEdit->text().isEmpty())
        s = s.arg("default");
    else
        s = s.arg(ui->lineEdit->text());
    if (ui->alsaChannelsSpinBox->value() > 0)
        s += QStringLiteral("?channels=%1").arg(ui->alsaChannelsSpinBox->value());
    Mlt::Producer *p = new Mlt::Producer(profile, s.toUtf8().constData());
    p->set(kBackgroundCaptureProperty, 1);
    p->set(kShotcutCaptionProperty, "ALSA");
    Settings.setAudioInput(ui->lineEdit->text());
    return p;
}

Mlt::Properties AlsaWidget::getPreset() const
{
    Mlt::Properties p;
    QString s("alsa:%1");
    if (ui->lineEdit->text().isEmpty())
        s = s.arg("default");
    else
        s = s.arg(ui->lineEdit->text());
    p.set("resource", s.toUtf8().constData());
    p.set("channels", ui->alsaChannelsSpinBox->value());
    return p;
}

void AlsaWidget::loadPreset(Mlt::Properties &p)
{
    QString s(p.get("resource"));
    int i = s.indexOf(':');
    if (i > -1)
        ui->lineEdit->setText(s.mid(i + 1));
    if (p.get("channels"))
        ui->alsaChannelsSpinBox->setValue(p.get_int("channels"));
}

void AlsaWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void AlsaWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}

void AlsaWidget::setProducer(Mlt::Producer *producer)
{
    ui->applyButton->show();
    if (producer)
        loadPreset(*producer);
}

void AlsaWidget::on_applyButton_clicked()
{
    MLT.setProducer(newProducer(MLT.profile()));
    MLT.play();
}
