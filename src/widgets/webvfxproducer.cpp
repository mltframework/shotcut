/*
 * Copyright (c) 2013-2017 Meltytech, LLC
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

#include "webvfxproducer.h"
#include "ui_webvfxproducer.h"
#include "mltcontroller.h"
#include "util.h"

static const char* kParamTransparent = "transparent";

WebvfxProducer::WebvfxProducer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WebvfxProducer)
{
    ui->setupUi(this);
    ui->reloadButton->hide();
    Util::setColorsToHighlight(ui->urlLabel);
}

WebvfxProducer::~WebvfxProducer()
{
    delete ui;
}

Mlt::Producer* WebvfxProducer::newProducer(Mlt::Profile &profile)
{
    QString s("webvfx:");
    if (!ui->webvfxCheckBox->isChecked())
        s.append("plain:");
    s.append(ui->urlLabel->text());
    Mlt::Producer* p = new Mlt::Producer(profile, s.toUtf8().constData());
    p->set(kParamTransparent, ui->transparentCheckBox->isChecked());
    return p;
}

void WebvfxProducer::setProducer(Mlt::Producer* producer)
{
    ui->reloadButton->show();
    if (producer) {
        QString resource(producer->get("resource"));
        if (resource.startsWith("plain:")) {
            resource.remove(0, 6);
            ui->webvfxCheckBox->setChecked(false);
        } else {
            ui->webvfxCheckBox->setChecked(true);
        }
        ui->urlLabel->setText(resource);
        ui->transparentCheckBox->setChecked(producer->get_int(kParamTransparent));
    }
}

void WebvfxProducer::on_reloadButton_clicked()
{
    Mlt::Producer* p = newProducer(MLT.profile());
    Mlt::Controller::copyFilters(*m_producer, *p);
    MLT.setProducer(p);
    MLT.play();
}
