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

#include "lissajouswidget.h"
#include "ui_lissajouswidget.h"

LissajousWidget::LissajousWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LissajousWidget),
    m_producer(0)
{
    ui->setupUi(this);
}

LissajousWidget::~LissajousWidget()
{
    delete ui;
    delete m_producer;
}

void LissajousWidget::on_xratioDial_valueChanged(int value)
{
    if (m_producer)
        m_producer->set("ratiox", value/100.0);
    emit producerChanged();
    ui->xratioSpinner->setValue(value/100.0);
}

void LissajousWidget::on_xratioSpinner_valueChanged(double value)
{
    ui->xratioDial->setValue(value * 100);
}

void LissajousWidget::on_yratioDial_valueChanged(int value)
{
    if (m_producer)
        m_producer->set("ratioy", value/100.0);
    emit producerChanged();
    ui->yratioSpinner->setValue(value/100.0);
}

void LissajousWidget::on_yratioSpinner_valueChanged(double value)
{
    ui->yratioDial->setValue(value * 100);
}

Mlt::Producer* LissajousWidget::producer(Mlt::Profile& profile)
{
    Mlt::Producer* p = new Mlt::Producer(profile, "frei0r.lissajous0r");
    p->set("ratiox", ui->xratioSpinner->text().toAscii().constData());
    p->set("ratioy", ui->yratioSpinner->text().toAscii().constData());
    return p;
}

Mlt::Properties* LissajousWidget::getPreset() const
{
    Mlt::Properties* p = new Mlt::Properties;
    p->set("ratiox", ui->xratioSpinner->text().toAscii().constData());
    p->set("ratioy", ui->yratioSpinner->text().toAscii().constData());
    return p;
}

void LissajousWidget::loadPreset(Mlt::Properties& p)
{
    ui->xratioSpinner->setValue(p.get_double("ratiox"));
    ui->yratioSpinner->setValue(p.get_double("ratioy"));
}

void LissajousWidget::setProducer(Mlt::Producer* producer)
{
    delete m_producer;
    m_producer = 0;
    ui->xratioSpinner->setValue(producer->get_double("ratiox"));
    ui->yratioSpinner->setValue(producer->get_double("ratioy"));
    m_producer = new Mlt::Producer(producer);
}
