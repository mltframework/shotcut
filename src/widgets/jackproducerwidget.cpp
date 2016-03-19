/*
 * Copyright (c) 2012-2016 Meltytech, LLC
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

#include "jackproducerwidget.h"
#include "ui_jackproducerwidget.h"
#include "util.h"

JackProducerWidget::JackProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::JackProducerWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label_2);
}

JackProducerWidget::~JackProducerWidget()
{
    delete ui;
}

Mlt::Producer* JackProducerWidget::producer(Mlt::Profile& profile)
{
    return new Mlt::Producer(profile, "jack:Shotcut");
}
