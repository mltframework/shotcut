/*
 * Copyright (c) 2012 Meltytech, LLC
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

#include "colorbarswidget.h"
#include "ui_colorbarswidget.h"

static const char* kParamType = "0";

ColorBarsWidget::ColorBarsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ColorBarsWidget)
{
    ui->setupUi(this);
    ui->comboBox->setCurrentIndex(4);
}

ColorBarsWidget::~ColorBarsWidget()
{
    delete ui;
}

Mlt::Producer* ColorBarsWidget::producer(Mlt::Profile& profile)
{
    Mlt::Producer* p = new Mlt::Producer(profile, "blipflash");
    p->set(kParamType, ui->comboBox->currentIndex());
    return p;
}

Mlt::Properties* ColorBarsWidget::getPreset() const
{
    Mlt::Properties* p = new Mlt::Properties;
    p->set(kParamType, ui->comboBox->currentIndex());
    return p;
}

void ColorBarsWidget::loadPreset(Mlt::Properties& p)
{
    ui->comboBox->setCurrentIndex(p.get_int(kParamType));
}

void ColorBarsWidget::on_comboBox_activated(int index)
{
    if (m_producer) {
        m_producer->set(kParamType, index);
        emit producerChanged();
    }
}
