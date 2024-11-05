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

#include "video4linuxwidget.h"
#include "ui_video4linuxwidget.h"
#include "pulseaudiowidget.h"
#include "jackproducerwidget.h"
#include "alsawidget.h"
#include "mltcontroller.h"
#include "util.h"
#include "shotcut_mlt_properties.h"
#include "settings.h"
#include <QtWidgets>

Video4LinuxWidget::Video4LinuxWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Video4LinuxWidget),
    m_audioWidget(0)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label_3);
    ui->applyButton->hide();
    ui->preset->saveDefaultPreset(getPreset());
    ui->preset->loadPresets();
    ui->v4lLineEdit->setText(Settings.videoInput());
}

Video4LinuxWidget::~Video4LinuxWidget()
{
    delete ui;
}

QString Video4LinuxWidget::URL() const
{
    QString s = QStringLiteral("video4linux2:%1?width=%2&height=%3")
                .arg(ui->v4lLineEdit->text())
                .arg(ui->v4lWidthSpinBox->value())
                .arg(ui->v4lHeightSpinBox->value());
    if (ui->v4lFramerateSpinBox->value() > 0)
        s += QStringLiteral("&framerate=%1").arg(ui->v4lFramerateSpinBox->value());
    if (ui->v4lStandardCombo->currentIndex() > 0)
        s += QStringLiteral("&standard=") + ui->v4lStandardCombo->currentText();
    if (ui->v4lChannelSpinBox->value() > 0)
        s += QStringLiteral("&channel=%1").arg(ui->v4lChannelSpinBox->value());
    return s;
}

Mlt::Producer *Video4LinuxWidget::newProducer(Mlt::Profile &profile)
{
    if (!profile.is_explicit()) {
        Mlt::Profile ntscProfile("dv_ntsc");
        Mlt::Profile palProfile("dv_pal");
        if (ui->v4lWidthSpinBox->value() == ntscProfile.width()
                && ui->v4lHeightSpinBox->value() == ntscProfile.height()) {
            profile.set_sample_aspect(ntscProfile.sample_aspect_num(), ntscProfile.sample_aspect_den());
            profile.set_progressive(ntscProfile.progressive());
            profile.set_colorspace(ntscProfile.colorspace());
            profile.set_frame_rate(ntscProfile.frame_rate_num(), ntscProfile.frame_rate_den());
        } else if (ui->v4lWidthSpinBox->value() == palProfile.width()
                   && ui->v4lHeightSpinBox->value() == palProfile.height()) {
            profile.set_sample_aspect(palProfile.sample_aspect_num(), palProfile.sample_aspect_den());
            profile.set_progressive(palProfile.progressive());
            profile.set_colorspace(palProfile.colorspace());
            profile.set_frame_rate(palProfile.frame_rate_num(), palProfile.frame_rate_den());
        } else {
            profile.set_width(ui->v4lWidthSpinBox->value());
            profile.set_height(ui->v4lHeightSpinBox->value());
            profile.set_sample_aspect(1, 1);
            profile.set_progressive(1);
            profile.set_colorspace(601);
            profile.set_frame_rate(ui->v4lFramerateSpinBox->value() * 10000, 10000);
        }
        MLT.updatePreviewProfile();
        MLT.setPreviewScale(Settings.playerPreviewScale());
    }
    Mlt::Producer *p = new Mlt::Producer(profile, URL().toLatin1().constData());
    if (!p->is_valid()) {
        delete p;
        p = new Mlt::Producer(profile, "color:");
        p->set("resource1", QStringLiteral("video4linux2:%1")
               .arg(ui->v4lLineEdit->text()).toLatin1().constData());
        p->set("error", 1);
    } else if (m_audioWidget) {
        Mlt::Producer *audio = dynamic_cast<AbstractProducerWidget *>(m_audioWidget)->newProducer(profile);
        Mlt::Tractor *tractor = new Mlt::Tractor;
        tractor->set("_profile", profile.get_profile(), 0);
        tractor->set_track(*p, 0);
        delete p;
        tractor->set_track(*audio, 1);
        delete audio;
        p = new Mlt::Producer(tractor->get_producer());
        delete tractor;
        p->set("resource1", QStringLiteral("video4linux2:%1")
               .arg(ui->v4lLineEdit->text()).toLatin1().constData());
    }
    p->set("device", ui->v4lLineEdit->text().toLatin1().constData());
    p->set("width", ui->v4lWidthSpinBox->value());
    p->set("height", ui->v4lHeightSpinBox->value());
    if (ui->v4lFramerateSpinBox->value() > 0)
        p->set("framerate", ui->v4lFramerateSpinBox->value());
    p->set("standard", ui->v4lStandardCombo->currentText().toLatin1().constData());
    p->set("channel", ui->v4lChannelSpinBox->value());
    p->set("audio_ix", ui->v4lAudioComboBox->currentIndex());
    p->set("force_seekable", 0);
    p->set(kBackgroundCaptureProperty, 1);
    p->set(kShotcutCaptionProperty, "Video4Linux");
    Settings.setVideoInput(ui->v4lLineEdit->text());
    return p;
}

Mlt::Properties Video4LinuxWidget::getPreset() const
{
    Mlt::Properties p;
    p.set("device", ui->v4lLineEdit->text().toLatin1().constData());
    p.set("width", ui->v4lWidthSpinBox->value());
    p.set("height", ui->v4lHeightSpinBox->value());
    p.set("framerate", ui->v4lFramerateSpinBox->value());
    p.set("standard", ui->v4lStandardCombo->currentText().toLatin1().constData());
    p.set("channel", ui->v4lChannelSpinBox->value());
    p.set("audio_ix", ui->v4lAudioComboBox->currentIndex());
    return p;
}

void Video4LinuxWidget::loadPreset(Mlt::Properties &p)
{
    ui->v4lLineEdit->setText(p.get("device"));
    ui->v4lWidthSpinBox->setValue(p.get_int("width"));
    ui->v4lHeightSpinBox->setValue(p.get_int("height"));
    ui->v4lFramerateSpinBox->setValue(p.get_double("framerate"));
    QString s(p.get("standard"));
    for (int i = 0; i < ui->v4lStandardCombo->count(); i++) {
        if (ui->v4lStandardCombo->itemText(i) == s) {
            ui->v4lStandardCombo->setCurrentIndex(i);
            break;
        }
    }
    ui->v4lChannelSpinBox->setValue(p.get_int("channel"));
    ui->v4lAudioComboBox->setCurrentIndex(p.get_int("audio_ix"));
    on_v4lAudioComboBox_activated(p.get_int("audio_ix"));
}

void Video4LinuxWidget::on_v4lAudioComboBox_activated(int index)
{
    if (m_audioWidget)
        delete m_audioWidget;
    m_audioWidget = 0;
    if (index == 1)
        m_audioWidget = new PulseAudioWidget(this);
    else if (index == 2)
        m_audioWidget = new JackProducerWidget(this);
    else if (index == 3)
        m_audioWidget = new AlsaWidget(this);
    if (m_audioWidget)
        ui->audioLayout->addWidget(m_audioWidget);
}

void Video4LinuxWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void Video4LinuxWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}

void Video4LinuxWidget::setProducer(Mlt::Producer *producer)
{
    ui->applyButton->show();
    if (producer)
        loadPreset(*producer);
}

void Video4LinuxWidget::on_applyButton_clicked()
{
    MLT.close();
    AbstractProducerWidget::setProducer(0);
    emit producerChanged(0);
    QCoreApplication::processEvents();

    Mlt::Producer *p = newProducer(MLT.profile());
    AbstractProducerWidget::setProducer(p);
    MLT.setProducer(p);
    MLT.play();
    emit producerChanged(p);
}
