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
        ui->colorLabel->setStyleSheet(QString("background-color: %1").arg(dialog.currentColor().name()));
    }
}

Mlt::Properties* ColorProducerWidget::mltProperties()
{
    Mlt::Properties* p = new Mlt::Properties;
    p->set("colour", ui->colorLabel->text().toAscii().constData());
    return p;    
}

void ColorProducerWidget::load(Mlt::Properties& p)
{
    ui->colorLabel->setText(p.get("colour"));
    ui->colorLabel->setStyleSheet(QString("background-color: %1")
        .arg(QString(p.get("colour")).replace(0, 3, "#")));
}
