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

void Video4LinuxWidget::load(Mlt::Properties& p)
{
    QString s(p.get("URL"));
    s = s.mid(s.indexOf(':') + 1); // chomp video4linux2:
    QString device = s.mid(0, s.indexOf('?'));
    ui->v4lLineEdit->setText(device);
    s = s.mid(s.indexOf('?') + 1); // chomp device
    if (s.indexOf('&') != -1)
    foreach (QString item, s.split('&')) {
        if (item.indexOf('=') != -1) {
            QStringList pair = item.split('=');
            if (pair.at(0) == "width")
                ui->v4lWidthSpinBox->setValue(pair.at(1).toInt());
            else if (pair.at(0) == "height")
                ui->v4lHeightSpinBox->setValue(pair.at(1).toInt());
            else if (pair.at(0) == "framerate")
                ui->v4lFramerateSpinBox->setValue(pair.at(1).toDouble());
            else if (pair.at(0) == "channel")
                ui->v4lChannelSpinBox->setValue(pair.at(1).toInt());
            else if (pair.at(0) == "standard") {
                for (int i = 0; i < ui->v4lStandardCombo->count(); i++) {
                    if (ui->v4lStandardCombo->itemText(i) == pair.at(1)) {
                        ui->v4lStandardCombo->setCurrentIndex(i);
                        break;
                    }
                }
            }
        }
    }
}
