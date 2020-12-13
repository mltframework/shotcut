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
#include "mltcontroller.h"

#include <QPushButton>

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
    connect(ui->fpsCheckBox, SIGNAL(toggled(bool)), ui->fpsWidget, SLOT(setEnabled(bool)));
    connect(ui->fpsCheckBox, SIGNAL(toggled(bool)), ui->fpsLabel, SLOT(setEnabled(bool)));
    ui->fpsCheckBox->setChecked(false);
    ui->fpsWidget->setEnabled(false);
    ui->fpsLabel->setEnabled(false);

    ui->fpsWidget->setFps(MLT.profile().fps());

    ui->frcComboBox->addItem(tr("Duplicate (fast)"), QVariant("dup"));
    ui->frcComboBox->addItem(tr("Blend"), QVariant("blend"));
    ui->frcComboBox->addItem(tr("Motion Compensation (slow)"), QVariant("mci"));
    ui->frcComboBox->setCurrentIndex(0);

    QPushButton* advancedButton = new QPushButton(tr("Advanced"));
    advancedButton->setCheckable(true);
    connect(advancedButton, SIGNAL(toggled(bool)), ui->advancedWidget, SLOT(setVisible(bool)));
    ui->advancedWidget->hide();
    advancedButton->setChecked(false);
    ui->buttonBox->addButton(advancedButton, QDialogButtonBox::ActionRole);

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

bool TranscodeDialog::fpsOverride() const
{
    return ui->fpsCheckBox->isChecked();
}

double TranscodeDialog::fps() const
{
    return ui->fpsWidget->fps();
}

QString TranscodeDialog::frc() const
{
    // Frame Rate Conversion Mode
    return ui->frcComboBox->currentData().toString();
}

bool TranscodeDialog::get709Convert()
{
    return ui->convert709CheckBox->isChecked();
}

void TranscodeDialog::set709Convert(bool enable)
{
    ui->convert709CheckBox->setChecked(enable);
}

void TranscodeDialog::on_horizontalSlider_valueChanged(int position)
{
    switch (position) {
    case 0:
        ui->formatLabel->setText(tr("Lossy: I-frame–only %1").arg("H.264/AC-3 MP4"));
        break;
    case 1:
        ui->formatLabel->setText(tr("Intermediate: %1").arg(m_isProgressive? "DNxHR/ALAC MOV" : "ProRes/ALAC MOV"));
        break;
    case 2:
        ui->formatLabel->setText(tr("Lossless: %1").arg("Ut Video/PCM MKV"));
        break;
    }
    m_format = position;
}

void TranscodeDialog::on_checkBox_clicked(bool checked)
{
    m_isChecked = checked;
}
