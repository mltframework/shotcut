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

#include "alsawidget.h"
#include "ui_alsawidget.h"

AlsaWidget::AlsaWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AlsaWidget)
{
    ui->setupUi(this);
}

AlsaWidget::~AlsaWidget()
{
    delete ui;
}

Mlt::Producer* AlsaWidget::producer(Mlt::Profile& profile)
{
    QString s("alsa:%1");
    if (ui->lineEdit->text().isEmpty())
        s = s.arg("default");
    else
        s = s.arg(ui->lineEdit->text());
    return new Mlt::Producer(profile, s.toUtf8().constData());
}

Mlt::Properties* AlsaWidget::getPreset() const
{
    Mlt::Properties* p = new Mlt::Properties;
    QString s("alsa:%1");
    if (ui->lineEdit->text().isEmpty())
        s = s.arg("default");
    else
        s = s.arg(ui->lineEdit->text());
    p->set("resource", s.toUtf8().constData());
    return p;
}

void AlsaWidget::loadPreset(Mlt::Properties& p)
{
    QString s(p.get("resource"));
    int i = s.indexOf(':');
    if (i > -1)
        ui->lineEdit->setText(s.mid(i + 1));
}
