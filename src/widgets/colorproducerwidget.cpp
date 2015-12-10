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

#include <QColorDialog>
#include "colorproducerwidget.h"
#include "ui_colorproducerwidget.h"

ColorProducerWidget::ColorProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ColorProducerWidget)
{
    ui->setupUi(this);
    ui->preset->saveDefaultPreset(*getPreset());
    ui->preset->loadPresets();
}

ColorProducerWidget::~ColorProducerWidget()
{
    delete ui;
}

void ColorProducerWidget::on_colorButton_clicked()
{
    QColorDialog dialog;
    dialog.setOption(QColorDialog::ShowAlphaChannel);
    if (dialog.exec() == QDialog::Accepted) {
        ui->colorLabel->setText(QString().sprintf("#%02X%02X%02X%02X",
                                                  qAlpha(dialog.currentColor().rgba()),
                                                  qRed(dialog.currentColor().rgba()),
                                                  qGreen(dialog.currentColor().rgba()),
                                                  qBlue(dialog.currentColor().rgba())
                                                  ));
        ui->colorLabel->setStyleSheet(QString("color: %1; background-color: %2")
                                      .arg((dialog.currentColor().value() < 150)? "white":"black")
                                      .arg(dialog.currentColor().name()));
        if (m_producer) {
            m_producer->set("resource", ui->colorLabel->text().toLatin1().constData());
            emit producerChanged();
        }
    }
}

Mlt::Producer* ColorProducerWidget::producer(Mlt::Profile& profile)
{
    Mlt::Producer* p = new Mlt::Producer(profile, "color:");
    p->set("resource", ui->colorLabel->text().toLatin1().constData());
    p->set("shotcut:caption", ui->colorLabel->text().toLatin1().constData());
    p->set("shotcut:detail", ui->colorLabel->text().toLatin1().constData());
    return p;
}

Mlt::Properties* ColorProducerWidget::getPreset() const
{
    Mlt::Properties* p = new Mlt::Properties;
    p->set("resource", ui->colorLabel->text().toLatin1().constData());
    return p;
}

void ColorProducerWidget::loadPreset(Mlt::Properties& p)
{
    QString color(p.get("resource"));
    ui->colorLabel->setText(color);
    color.replace(0, 3, "#");
    ui->colorLabel->setStyleSheet(QString("color: %1; background-color: %2")
        .arg((QColor(color).value() < 150)? "white":"black")
        .arg(color));
}

void ColorProducerWidget::on_preset_selected(void* p)
{
    Mlt::Properties* properties = (Mlt::Properties*) p;
    loadPreset(*properties);
    delete properties;
}

void ColorProducerWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}
