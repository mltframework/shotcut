/*
 * Copyright (c) 2020 Meltytech, LLC
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

#include "systemsyncdialog.h"
#include "ui_systemsyncdialog.h"
#include "settings.h"
#include "mltcontroller.h"

SystemSyncDialog::SystemSyncDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SystemSyncDialog),
    m_oldValue(Settings.playerVideoDelayMs())
{
    ui->setupUi(this);
    ui->syncSlider->setValue(Settings.playerVideoDelayMs());
}

SystemSyncDialog::~SystemSyncDialog()
{
    delete ui;
}

void SystemSyncDialog::on_syncSlider_sliderReleased()
{
    Settings.setPlayerVideoDelayMs(ui->syncSlider->value());
    MLT.consumerChanged();
}

void SystemSyncDialog::on_syncSpinBox_editingFinished()
{
    ui->syncSlider->setValue(ui->syncSpinBox->value());
    Settings.setPlayerVideoDelayMs(ui->syncSpinBox->value());
    MLT.consumerChanged();
}

void SystemSyncDialog::on_buttonBox_rejected()
{
    Settings.setPlayerVideoDelayMs(m_oldValue);
    MLT.consumerChanged();
}

void SystemSyncDialog::on_undoButton_clicked()
{
    ui->syncSlider->setValue(0);
    Settings.setPlayerVideoDelayMs(0);
    MLT.consumerChanged();
}
