/*
 * Copyright (c) 2017-2019 Meltytech, LLC
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

#include "transcodedialog.h"
#include "ui_transcodedialog.h"

TranscodeDialog::TranscodeDialog(const QString& message, bool isProgressive, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TranscodeDialog),
    m_format(1),
    m_isChecked(false),
    m_isProgressive(isProgressive)
{
    ui->setupUi(this);
    setWindowTitle(tr("Convert to Edit-friendly..."));
    ui->messageLabel->setText(message);
    ui->checkBox->hide();
    on_horizontalSlider_valueChanged(m_format);
}

TranscodeDialog::~TranscodeDialog()
{
    delete ui;
}

void TranscodeDialog::showCheckBox()
{
    ui->checkBox->show();
}

void TranscodeDialog::on_horizontalSlider_valueChanged(int position)
{
    switch (position) {
    case 0:
        ui->formatLabel->setText(tr("Lossy: I-frame–only H.264/AC-3 MP4"));
        break;
    case 1:
        ui->formatLabel->setText(tr("Intermediate: %1/ALAC MOV").arg(m_isProgressive? "DNxHR" : "ProRes"));
        break;
    case 2:
        ui->formatLabel->setText(tr("Lossless: Ut Video/FLAC MKV"));
        break;
    }
    m_format = position;
}

void TranscodeDialog::on_checkBox_clicked(bool checked)
{
    m_isChecked = checked;
}
