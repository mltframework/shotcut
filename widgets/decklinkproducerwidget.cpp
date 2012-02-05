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

#include "decklinkproducerwidget.h"
#include "ui_decklinkproducerwidget.h"

DecklinkProducerWidget::DecklinkProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DecklinkProducerWidget)
{
    ui->setupUi(this);
}

DecklinkProducerWidget::~DecklinkProducerWidget()
{
    delete ui;
}

Mlt::Producer* DecklinkProducerWidget::producer(Mlt::Profile& profile)
{
    Mlt::Producer* p = new Mlt::Producer(profile,
        QString("decklink:%1").arg(ui->decklinkCardSpinner->value()).toAscii().constData());
    return p;
}

Mlt::Properties* DecklinkProducerWidget::getPreset() const
{
    Mlt::Properties* p = new Mlt::Properties;
    p->set("card", ui->decklinkCardSpinner->value());
    return p;
}

void DecklinkProducerWidget::loadPreset(Mlt::Properties& p)
{
    ui->decklinkCardSpinner->setValue(p.get_int("card"));
}
