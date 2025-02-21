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

#include "mltcontroller.h"
#include "settings.h"

SystemSyncDialog::SystemSyncDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SystemSyncDialog)
    , m_oldValue(Settings.playerVideoDelayMs())
{
    ui->setupUi(this);
    ui->syncSlider->setValue(Settings.playerVideoDelayMs());
    ui->applyButton->hide();
}

SystemSyncDialog::~SystemSyncDialog()
{
    delete ui;
}

void SystemSyncDialog::on_syncSlider_sliderReleased()
{
    setDelay(ui->syncSlider->value());
}

void SystemSyncDialog::on_syncSpinBox_editingFinished()
{
    ui->syncSlider->setValue(ui->syncSpinBox->value());
    setDelay(ui->syncSpinBox->value());
}

void SystemSyncDialog::on_buttonBox_rejected()
{
    setDelay(m_oldValue);
}

void SystemSyncDialog::on_undoButton_clicked()
{
    ui->syncSlider->setValue(0);
    setDelay(0);
}

void SystemSyncDialog::on_syncSpinBox_valueChanged(int arg1)
{
    Q_UNUSED(arg1)
    ui->applyButton->show();
}

void SystemSyncDialog::on_applyButton_clicked()
{
    setDelay(ui->syncSpinBox->value());
}

void SystemSyncDialog::setDelay(int delay)
{
    if (delay != Settings.playerVideoDelayMs()) {
        Settings.setPlayerVideoDelayMs(delay);
        MLT.consumerChanged();
    }
    ui->applyButton->hide();
}
