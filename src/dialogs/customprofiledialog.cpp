/*
 * Copyright (c) 2013 Meltytech, LLC
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

#include "customprofiledialog.h"
#include "ui_customprofiledialog.h"
#include "mltcontroller.h"
#include <QDir>
#include <QDesktopServices>

CustomProfileDialog::CustomProfileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CustomProfileDialog)
{
    ui->setupUi(this);
    ui->widthSpinner->setValue(MLT.profile().width());
    ui->heightSpinner->setValue(MLT.profile().height());
    ui->aspectNumSpinner->setValue(MLT.profile().display_aspect_num());
    ui->aspectDenSpinner->setValue(MLT.profile().display_aspect_den());
    ui->fpsSpinner->setValue(MLT.profile().fps());
    ui->scanModeCombo->setCurrentIndex(MLT.profile().progressive());
    ui->colorspaceCombo->setCurrentIndex(MLT.profile().colorspace() == 709);
}

CustomProfileDialog::~CustomProfileDialog()
{
    delete ui;
}

QString CustomProfileDialog::profileName() const
{
    return ui->nameEdit->text();
}

void CustomProfileDialog::on_buttonBox_accepted()
{
    // Save it to a file
    if (ui->nameEdit->text().isEmpty())
        return;
    QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
    QString subdir("profiles");
    if (!dir.exists())
        dir.mkpath(dir.path());
    if (!dir.cd(subdir)) {
        if (dir.mkdir(subdir))
            dir.cd(subdir);
    }
    MLT.profile().set_explicit(1);
    MLT.profile().set_width(ui->widthSpinner->value());
    MLT.profile().set_height(ui->heightSpinner->value());
    int sar_num = ui->heightSpinner->value() * ui->aspectNumSpinner->value() / ui->aspectDenSpinner->value();
    int sar_den = ui->widthSpinner->value();
    if (sar_num == sar_den)
        sar_num = sar_den = 1;
    MLT.profile().set_sample_aspect(sar_num, sar_den);
    switch (int(ui->fpsSpinner->value() * 10)) {
    case 239:
        MLT.profile().set_frame_rate(24000, 1001);
        break;
    case 299:
        MLT.profile().set_frame_rate(30000, 1001);
        break;
    case 479:
        MLT.profile().set_frame_rate(48000, 1001);
        break;
    case 599:
        MLT.profile().set_frame_rate(60000, 1001);
        break;
    default:
        MLT.profile().set_frame_rate(ui->fpsSpinner->value() * 1000, 1000);
        break;
    }
    MLT.profile().set_progressive(ui->scanModeCombo->currentIndex());
    MLT.profile().set_colorspace((ui->colorspaceCombo->currentIndex() == 1)? 709 : 601);
    Mlt::Properties p;
    p.set("width", MLT.profile().width());
    p.set("height", MLT.profile().height());
    p.set("sample_aspect_num", MLT.profile().sample_aspect_num());
    p.set("sample_aspect_den", MLT.profile().sample_aspect_den());
    p.set("display_aspect_num", MLT.profile().display_aspect_num());
    p.set("display_aspect_den", MLT.profile().display_aspect_den());
    p.set("progressive", MLT.profile().progressive());
    p.set("colorspace", MLT.profile().colorspace());
    p.set("frame_rate_num", MLT.profile().frame_rate_num());
    p.set("frame_rate_den", MLT.profile().frame_rate_den());
    p.save(dir.filePath(ui->nameEdit->text()).toUtf8().constData());
}
