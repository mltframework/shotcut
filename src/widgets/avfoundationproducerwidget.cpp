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

#include "avfoundationproducerwidget.h"
#include "ui_avfoundationproducerwidget.h"
#include "mltcontroller.h"
#include "util.h"
#include "settings.h"
#include <QCameraInfo>
#include <QCamera>
#include <QString>
#include <QAudioDeviceInfo>
#include <QDesktopWidget>
#include "shotcut_mlt_properties.h"
#include <Logger.h>

#define ENABLE_SCREEN_CAPTURE (0)

AvfoundationProducerWidget::AvfoundationProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AvfoundationProducerWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label);

#ifdef Q_OS_MAC
    auto currentVideo = 1;
    for (const auto& cameraInfo : QCameraInfo::availableCameras()) {
        if (Settings.videoInput() == cameraInfo.description()) {
            currentVideo = ui->videoCombo->count();
        }
        ui->videoCombo->addItem(cameraInfo.description(), cameraInfo.deviceName().toLocal8Bit());
    }
#if ENABLE_SCREEN_CAPTURE
    for (int i = 0; i < QApplication::desktop()->screenCount(); i++)
        ui->videoCombo->addItem(QString("Capture screen %1").arg(i));
#endif
    auto currentAudio = 1;
    for (const auto& deviceInfo : QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (Settings.audioInput() == deviceInfo.deviceName()) {
            currentAudio = ui->audioCombo->count();
        }
        ui->audioCombo->addItem(deviceInfo.deviceName());
    }
    if (ui->videoCombo->count() > 1)
        ui->videoCombo->setCurrentIndex(currentVideo);
    if (ui->audioCombo->count() > 1) {
        ui->audioCombo->setCurrentIndex(currentAudio);
    }
#endif
}

AvfoundationProducerWidget::~AvfoundationProducerWidget()
{
    delete ui;
}

Mlt::Producer *AvfoundationProducerWidget::newProducer(Mlt::Profile& profile)
{
    QString resource;
    qreal frameRate = 30.0;
    QSize size {1280, 720};
    Util::cameraFrameRateSize(ui->videoCombo->currentData().toByteArray(), frameRate, size);
    if (ui->videoCombo->currentIndex()) {
        resource = QString("avfoundation:%1:%2?pixel_format=yuyv422&framerate=%3&video_size=%4x%5")
            .arg(ui->videoCombo->currentText().replace(tr("None"), "none"))
            .arg(ui->audioCombo->currentText().replace(tr("None"), "none"))
            .arg(frameRate).arg(size.width()).arg(size.height());
    } else {
        resource = QString("avfoundation:none:%1").arg(ui->audioCombo->currentText().replace(tr("None"), "none"));
    }
    LOG_DEBUG() << resource;
    Mlt::Producer* p = new Mlt::Producer(profile, resource.toUtf8().constData());
    if (!p || !p->is_valid()) {
        delete p;
        p = new Mlt::Producer(profile, "color:");
        p->set("resource", QString("avfoundation:%1:%2")
               .arg(ui->videoCombo->currentText().replace(tr("None"), "none"))
               .arg(ui->audioCombo->currentText())
               .toUtf8().constData());
        p->set("error", 1);
    }
    p->set("force_seekable", 0);
    p->set(kBackgroundCaptureProperty, 1);
    p->set(kShotcutCaptionProperty, tr("Audio/Video Device").toUtf8().constData());
    if (ui->audioCombo->currentIndex() > 0) {
        Settings.setAudioInput(ui->audioCombo->currentText());
    }
    if (ui->videoCombo->currentIndex() > 0) {
        Settings.setVideoInput(ui->videoCombo->currentText());
    }
    return p;
}

void AvfoundationProducerWidget::setProducer(Mlt::Producer *producer)
{
    QStringList resource = QString(producer->get("resource")).split('?');
    resource = resource[0].split(':');
    ui->videoCombo->setCurrentIndex(0);
    ui->audioCombo->setCurrentIndex(0);
    if (resource.size() > 2) {
        for (int i = 1; i < ui->videoCombo->count(); i++) {
            if (ui->videoCombo->itemText(i) == resource[1]) {
                ui->videoCombo->setCurrentIndex(i);
                break;
            }
        }
        for (int i = 1; i < ui->audioCombo->count(); i++) {
            if (ui->audioCombo->itemText(i) == resource[2]) {
                ui->audioCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    AbstractProducerWidget::setProducer(producer);
}

void AvfoundationProducerWidget::on_videoCombo_activated(int index)
{
    Q_UNUSED(index)
    if (m_producer) {
        MLT.close();
        AbstractProducerWidget::setProducer(0);
        emit producerChanged(0);
        QCoreApplication::processEvents();

        Mlt::Producer* p = newProducer(MLT.profile());
        AbstractProducerWidget::setProducer(p);
        MLT.setProducer(p);
        MLT.play();
        emit producerChanged(p);
    }
}

void AvfoundationProducerWidget::on_audioCombo_activated(int index)
{
    on_videoCombo_activated(index);
}
