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

#include "video4linuxwidget.h"
#include "ui_video4linuxwidget.h"

Video4LinuxWidget::Video4LinuxWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Video4LinuxWidget)
{
    ui->setupUi(this);
}

Video4LinuxWidget::~Video4LinuxWidget()
{
    delete ui;
}

QString Video4LinuxWidget::URL() const
{
    QString s = QString("video4linux2:%1?width=%2&height=%3&framerate=%4")
            .arg(ui->v4lLineEdit->text())
            .arg(ui->v4lWidthSpinBox->value())
            .arg(ui->v4lHeightSpinBox->value())
            .arg(ui->v4lFramerateSpinBox->value());
    if (ui->v4lStandardCombo->currentIndex() > 0)
        s += QString("&standard=") + ui->v4lStandardCombo->currentText();
    if (ui->v4lChannelSpinBox->value() > 0)
        s += QString("&channel=%1").arg(ui->v4lChannelSpinBox->value());
    return s;
}

Mlt::Producer* Video4LinuxWidget::producer(Mlt::Profile& profile)
{
    Mlt::Producer* p = new Mlt::Producer(profile, URL().toAscii().constData());
    if (!p->is_valid()) {
        delete p;
        p = new Mlt::Producer(profile, "color:");
        p->set("resource", QString("video4linux2:%1")
               .arg(ui->v4lLineEdit->text()).toAscii().constData());
        p->set("error", 1);
    }
    p->set("device", ui->v4lLineEdit->text().toAscii().constData());
    p->set("width", ui->v4lWidthSpinBox->value());
    p->set("height", ui->v4lHeightSpinBox->value());
    p->set("framerate", ui->v4lFramerateSpinBox->value());
    p->set("standard", ui->v4lStandardCombo->currentText().toAscii().constData());
    p->set("channel", ui->v4lChannelSpinBox->value());
    return p;
}

Mlt::Properties* Video4LinuxWidget::getPreset() const
{
    Mlt::Properties* p = new Mlt::Properties;
    p->set("device", ui->v4lLineEdit->text().toAscii().constData());
    p->set("width", ui->v4lWidthSpinBox->value());
    p->set("height", ui->v4lHeightSpinBox->value());
    p->set("framerate", ui->v4lFramerateSpinBox->value());
    p->set("standard", ui->v4lStandardCombo->currentText().toAscii().constData());
    p->set("channel", ui->v4lChannelSpinBox->value());
    return p;
}

void Video4LinuxWidget::loadPreset(Mlt::Properties& p)
{
    ui->v4lLineEdit->setText(p.get("device"));
    ui->v4lWidthSpinBox->setValue(p.get_int("width"));
    ui->v4lHeightSpinBox->setValue(p.get_int("height"));
    ui->v4lFramerateSpinBox->setValue(p.get_double("framerate"));
    QString s(p.get("standard"));
    for (int i = 0; i < ui->v4lStandardCombo->count(); i++) {
        if (ui->v4lStandardCombo->itemText(i) == s) {
            ui->v4lStandardCombo->setCurrentIndex(i);
            break;
        }
    }
    ui->v4lChannelSpinBox->setValue(p.get_int("channel"));
}
