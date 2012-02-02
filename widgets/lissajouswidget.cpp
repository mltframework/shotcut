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
    ui(new Ui::LissajousWidget)
{
    ui->setupUi(this);
}

LissajousWidget::~LissajousWidget()
{
    delete ui;
}

void LissajousWidget::on_xratioDial_valueChanged(int value)
{
    ui->xratioSpinner->setValue(value/100.0);
}

void LissajousWidget::on_xratioSpinner_valueChanged(double value)
{
    ui->xratioDial->setValue(value * 100);
}

void LissajousWidget::on_yratioDial_valueChanged(int value)
{
    ui->yratioSpinner->setValue(value/100.0);    
}

void LissajousWidget::on_yratioSpinner_valueChanged(double value)
{
    ui->yratioDial->setValue(value * 100);
}

Mlt::Properties* LissajousWidget::mltProperties()
{
    Mlt::Properties* p = new Mlt::Properties;
    p->set("ratiox", ui->xratioSpinner->text().toAscii().constData());
    p->set("ratioy", ui->yratioSpinner->text().toAscii().constData());
    return p;
}

void LissajousWidget::load(Mlt::Properties& p)
{
    ui->xratioSpinner->setValue(p.get_double("ratiox"));
    ui->yratioSpinner->setValue(p.get_double("ratioy"));
}
