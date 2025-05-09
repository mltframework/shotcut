/*
 * Copyright (c) 2012-2025 Meltytech, LLC
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

#include "durationdialog.h"
#include "mltcontroller.h"
#include "ui_durationdialog.h"

DurationDialog::DurationDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DurationDialog)
{
    ui->setupUi(this);
    ui->spinBox->setMaximum(MLT.maxFrameCount());
    connect(ui->spinBox, &TimeSpinBox::accepted, this, &QDialog::accept);
}

DurationDialog::~DurationDialog()
{
    delete ui;
}

void DurationDialog::setDuration(int frames)
{
    ui->spinBox->setValue(frames);
}

int DurationDialog::duration() const
{
    return ui->spinBox->value();
}
