/*
 * Copyright (c) 2012-2022 Meltytech, LLC
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

#include "addencodepresetdialog.h"
#include "ui_addencodepresetdialog.h"

AddEncodePresetDialog::AddEncodePresetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddEncodePresetDialog)
{
    ui->setupUi(this);
}

AddEncodePresetDialog::~AddEncodePresetDialog()
{
    delete ui;
}

void AddEncodePresetDialog::setProperties(const QString &properties)
{
    ui->propertiesEdit->setPlainText(properties);
}

QString AddEncodePresetDialog::presetName() const
{
    return ui->nameEdit->text();
}

QString AddEncodePresetDialog::properties() const
{
    const auto &extension = ui->extensionEdit->text();
    if (!extension.isEmpty()) {
        return ui->propertiesEdit->toPlainText() + "\nmeta.preset.extension=" + extension;
    }
    return ui->propertiesEdit->toPlainText();
}
