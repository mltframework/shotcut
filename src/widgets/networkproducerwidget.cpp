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

#include "networkproducerwidget.h"
#include "ui_networkproducerwidget.h"
#include "mltcontroller.h"
#include "util.h"

NetworkProducerWidget::NetworkProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NetworkProducerWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label_2);
    ui->applyButton->hide();
    ui->preset->saveDefaultPreset(getPreset());
    ui->preset->loadPresets();
}

NetworkProducerWidget::~NetworkProducerWidget()
{
    delete ui;
}

Mlt::Producer *NetworkProducerWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile, ui->urlLineEdit->text().toUtf8().constData());
    return p;
}

Mlt::Properties NetworkProducerWidget::getPreset() const
{
    Mlt::Properties p;
    p.set("resource", ui->urlLineEdit->text().toUtf8().constData());
    return p;
}

void NetworkProducerWidget::loadPreset(Mlt::Properties &p)
{
    const char *resource = p.get("resource");
    if (qstrcmp(resource, "<tractor>") && qstrcmp(resource, "<playlist>"))
        ui->urlLineEdit->setText(resource);
}

void NetworkProducerWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void NetworkProducerWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}

void NetworkProducerWidget::setProducer(Mlt::Producer *producer)
{
    ui->applyButton->show();
    if (producer)
        loadPreset(*producer);
}

void NetworkProducerWidget::on_applyButton_clicked()
{
    MLT.setProducer(newProducer(MLT.profile()));
    MLT.play();
}
