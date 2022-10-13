/*
 * Copyright (c) 2013-2021 Meltytech, LLC
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
#include "settings.h"
#include "util.h"
#include <QDir>
#include <QDesktopServices>

CustomProfileDialog::CustomProfileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CustomProfileDialog),
    m_fps(0.0)
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
    MLT.profile().set_explicit(1);
    MLT.profile().set_width(ui->widthSpinner->value());
    MLT.profile().set_height(ui->heightSpinner->value());
    MLT.profile().set_display_aspect(ui->aspectNumSpinner->value(), ui->aspectDenSpinner->value());
    QSize sar(ui->aspectNumSpinner->value() * ui->heightSpinner->value(),
              ui->aspectDenSpinner->value() * ui->widthSpinner->value());
    auto gcd = Util::greatestCommonDivisor(sar.width(), sar.height());
    MLT.profile().set_sample_aspect(sar.width() / gcd, sar.height() / gcd);
    int numerator, denominator;
    Util::normalizeFrameRate(ui->fpsSpinner->value(), numerator, denominator);
    MLT.profile().set_frame_rate(numerator, denominator);
    MLT.profile().set_progressive(ui->scanModeCombo->currentIndex());
    MLT.profile().set_colorspace((ui->colorspaceCombo->currentIndex() == 1) ? 709 : 601);
    MLT.updatePreviewProfile();
    MLT.setPreviewScale(Settings.playerPreviewScale());

    // Save it to a file
    if (!ui->nameEdit->text().isEmpty()) {
        QDir dir(Settings.appDataLocation());
        QString subdir("profiles");
        if (!dir.exists())
            dir.mkpath(dir.path());
        if (!dir.cd(subdir)) {
            if (dir.mkdir(subdir))
                dir.cd(subdir);
        }
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
}

void CustomProfileDialog::on_widthSpinner_editingFinished()
{
    ui->widthSpinner->setValue(Util::coerceMultiple(ui->widthSpinner->value()));
}

void CustomProfileDialog::on_heightSpinner_editingFinished()
{
    ui->heightSpinner->setValue(Util::coerceMultiple(ui->heightSpinner->value()));
}

void CustomProfileDialog::on_fpsSpinner_editingFinished()
{
    if (ui->fpsSpinner->value() != m_fps) {
        const QString caption(tr("Video Mode Frames/sec"));
        if (ui->fpsSpinner->value() == 23.98 || ui->fpsSpinner->value() == 23.976) {
            Util::showFrameRateDialog(caption, 24000, ui->fpsSpinner, this);
        } else if (ui->fpsSpinner->value() == 29.97) {
            Util::showFrameRateDialog(caption, 30000, ui->fpsSpinner, this);
        } else if (ui->fpsSpinner->value() == 47.95) {
            Util::showFrameRateDialog(caption, 48000, ui->fpsSpinner, this);
        } else if (ui->fpsSpinner->value() == 59.94) {
            Util::showFrameRateDialog(caption, 60000, ui->fpsSpinner, this);
        }
        m_fps = ui->fpsSpinner->value();
    }
}

void CustomProfileDialog::on_fpsComboBox_activated(const QString &arg1)
{
    if (arg1.isEmpty()) return;
    ui->fpsSpinner->setValue(arg1.toDouble());
}

void CustomProfileDialog::on_resolutionComboBox_activated(const QString &arg1)
{
    if (arg1.isEmpty()) return;
    auto parts = arg1.split(' ');
    ui->widthSpinner->setValue(parts[0].toInt());
    ui->heightSpinner->setValue(parts[2].toInt());
}

void CustomProfileDialog::on_aspectRatioComboBox_activated(const QString &arg1)
{
    if (arg1.isEmpty()) return;
    auto parts = arg1.split(' ')[0].split(':');
    ui->aspectNumSpinner->setValue(parts[0].toInt());
    ui->aspectDenSpinner->setValue(parts[1].toInt());
}
