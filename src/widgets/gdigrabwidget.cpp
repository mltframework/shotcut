/*
 * Copyright (c) 2015-2022 Meltytech, LLC
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

#include "gdigrabwidget.h"
#include "ui_gdigrabwidget.h"
#include "mltcontroller.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include "settings.h"
#include <QtWidgets>
#include <QMediaDevices>
#include <QAudioDevice>

GDIgrabWidget::GDIgrabWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GDIgrabWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label_9);
    ui->applyButton->hide();
    const QRect &r = QGuiApplication::primaryScreen()->geometry();
    ui->widthSpinBox->setValue(r.size().width());
    ui->heightSpinBox->setValue(r.size().height());
    ui->preset->saveDefaultPreset(getPreset());
    ui->preset->loadPresets();

    if (QMediaDevices::audioInputs().count() > 0) {
        for (const auto &deviceInfo : QMediaDevices::audioInputs())
            ui->audioComboBox->addItem(deviceInfo.description());
    } else {
        ui->audioLabel->hide();
        ui->audioComboBox->hide();
    }
}

GDIgrabWidget::~GDIgrabWidget()
{
    delete ui;
}

QString GDIgrabWidget::URL(Mlt::Profile &profile) const
{
    if (!profile.is_explicit()) {
        profile.set_width(ui->widthSpinBox->value());
        profile.set_height(ui->heightSpinBox->value());
        profile.set_sample_aspect(1, 1);
        profile.set_progressive(1);
        profile.set_colorspace(709);
        profile.set_frame_rate(25, 1);
        MLT.updatePreviewProfile();
        MLT.setPreviewScale(Settings.playerPreviewScale());
    }
    QString s =
        QString("gdigrab:desktop?offset_x=%1&offset_y=%2&video_size=%3x%4&framerate=%5&show_region=%6&draw_mouse=%7")
        .arg(ui->xSpinBox->value())
        .arg(ui->ySpinBox->value())
        .arg(ui->widthSpinBox->value())
        .arg(ui->heightSpinBox->value())
        .arg(profile.fps())
        .arg(ui->showRegionCheckBox->isChecked() ? 1 : 0)
        .arg(ui->drawMouseCheckBox->isChecked() ? 1 : 0);
    return s;
}

Mlt::Producer *GDIgrabWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile, URL(profile).toLatin1().constData());
    if (!p->is_valid()) {
        delete p;
        p = new Mlt::Producer(profile, "color:");
        p->set("error", 1);
    } else if (ui->audioComboBox->currentIndex() > 0) {
        Mlt::Producer *audio = new Mlt::Producer(profile,
                                                 QString("dshow:audio=%1").arg(ui->audioComboBox->currentText())
                                                 .toLatin1().constData());
        Mlt::Tractor *tractor = new Mlt::Tractor;
        tractor->set("_profile", profile.get_profile(), 0);
        tractor->set_track(*p, 0);
        delete p;
        tractor->set_track(*audio, 1);
        delete audio;
        p = new Mlt::Producer(tractor->get_producer());
        delete tractor;
    }
    p->set("xpos", ui->xSpinBox->value());
    p->set("ypos", ui->ySpinBox->value());
    p->set("width", ui->widthSpinBox->value());
    p->set("height", ui->heightSpinBox->value());
    p->set("show_region", ui->showRegionCheckBox->isChecked() ? 1 : 0);
    p->set("draw_mouse", ui->drawMouseCheckBox->isChecked() ? 1 : 0);
    p->set("audio_ix", ui->audioComboBox->currentIndex());
    p->set(kBackgroundCaptureProperty, 1);
    p->set("force_seekable", 0);
    return p;
}

Mlt::Properties GDIgrabWidget::getPreset() const
{
    Mlt::Properties p;
    p.set("xpos", ui->xSpinBox->value());
    p.set("ypos", ui->ySpinBox->value());
    p.set("width", ui->widthSpinBox->value());
    p.set("height", ui->heightSpinBox->value());
    p.set("show_region", ui->showRegionCheckBox->isChecked() ? 1 : 0);
    p.set("draw_mouse", ui->drawMouseCheckBox->isChecked() ? 1 : 0);
    p.set("audio_ix", ui->audioComboBox->currentIndex());
    p.set(kBackgroundCaptureProperty, 1);
    return p;
}

void GDIgrabWidget::loadPreset(Mlt::Properties &p)
{
    ui->xSpinBox->setValue(p.get_int("xpos"));
    ui->ySpinBox->setValue(p.get_int("ypos"));
    ui->widthSpinBox->setValue(p.get_int("width"));
    ui->heightSpinBox->setValue(p.get_int("height"));
    ui->showRegionCheckBox->setChecked(p.get_int("show_region"));
    ui->drawMouseCheckBox->setChecked(p.get_int("draw_mouse"));
    ui->audioComboBox->setCurrentIndex(p.get_int("audio_ix"));
}

void GDIgrabWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void GDIgrabWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}

void GDIgrabWidget::setProducer(Mlt::Producer *producer)
{
    ui->applyButton->show();
    if (producer)
        loadPreset(*producer);
}

void GDIgrabWidget::on_applyButton_clicked()
{
    MLT.setProducer(newProducer(MLT.profile()));
    MLT.play();
}
