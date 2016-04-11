/*
 * Copyright (c) 2015-2016 Meltytech, LLC
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

#include "avfoundationproducerwidget.h"
#include "ui_avfoundationproducerwidget.h"
#include "mltcontroller.h"
#include "util.h"
#include <QCamera>
#include <QString>
#include <QAudioDeviceInfo>
#include <QDesktopWidget>
#include "shotcut_mlt_properties.h"
#include <Logger.h>

AvfoundationProducerWidget::AvfoundationProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AvfoundationProducerWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label);

#ifdef Q_OS_MAC
    foreach (const QByteArray &deviceName, QCamera::availableDevices())
        ui->videoCombo->addItem(QCamera::deviceDescription(deviceName));
    for (int i = 0; i < QApplication::desktop()->screenCount(); i++)
        ui->videoCombo->addItem(QString("Capture screen %1").arg(i));
    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
        ui->audioCombo->addItem(deviceInfo.deviceName());
    if (ui->videoCombo->count() > 1)
        ui->videoCombo->setCurrentIndex(1);
    if (ui->audioCombo->count() > 1)
        ui->audioCombo->setCurrentIndex(1);
#endif
}

AvfoundationProducerWidget::~AvfoundationProducerWidget()
{
    delete ui;
}

Mlt::Producer *AvfoundationProducerWidget::producer(Mlt::Profile& profile)
{
    QString resource = QString("avfoundation:%1:%2?pixel_format=yuyv422&framerate=30&video_size=1280x720")
            .arg(ui->videoCombo->currentText().replace(tr("None"), "none"))
            .arg(ui->audioCombo->currentText().replace(tr("None"), "none"));
    LOG_DEBUG() << resource;
    Mlt::Producer* p = new Mlt::Producer(profile, resource.toLatin1().constData());
    if (!p || !p->is_valid()) {
        delete p;
        p = new Mlt::Producer(profile, "color:");
        p->set("resource", QString("avfoundation:%1:%2")
               .arg(ui->videoCombo->currentText())
               .arg(ui->audioCombo->currentText())
               .toLatin1().constData());
        p->set("error", 1);
    }
    p->set("force_seekable", 0);
    p->set(kBackgroundCaptureProperty, ui->backgroundCheckBox->isChecked()? 1: 0);
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
                ui->backgroundCheckBox->setEnabled(resource[1].startsWith("Capture screen"));
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
    ui->backgroundCheckBox->setChecked(producer->get_int(kBackgroundCaptureProperty));
    AbstractProducerWidget::setProducer(producer);
}

void AvfoundationProducerWidget::on_videoCombo_activated(int index)
{
    Q_UNUSED(index)
    ui->backgroundCheckBox->setEnabled(ui->videoCombo->currentText().startsWith("Capture screen"));
    if (!ui->backgroundCheckBox->isEnabled())
        ui->backgroundCheckBox->setChecked(false);
    if (m_producer) {
        MLT.pause();
        delete m_producer;
        m_producer = new Mlt::Producer(producer(MLT.profile()));
        MLT.setProducer(m_producer);
        MLT.play();
        emit producerChanged(m_producer);
    }
}

void AvfoundationProducerWidget::on_audioCombo_activated(int index)
{
    on_videoCombo_activated(index);
}
