/*
 * Copyright (c) 2014-2016 Meltytech, LLC
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

#include "directshowvideowidget.h"
#include "ui_directshowvideowidget.h"
#include "mltcontroller.h"
#include "util.h"
#include <QCamera>
#include <QString>
#include <QAudioDeviceInfo>

DirectShowVideoWidget::DirectShowVideoWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DirectShowVideoWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label);
#ifdef Q_OS_WIN
    foreach (const QByteArray &deviceName, QCamera::availableDevices())
        ui->videoCombo->addItem(QCamera::deviceDescription(deviceName));
    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
        ui->audioCombo->addItem(deviceInfo.deviceName());
#endif
}

DirectShowVideoWidget::~DirectShowVideoWidget()
{
    delete ui;
}

Mlt::Producer *DirectShowVideoWidget::producer(Mlt::Profile& profile)
{
#if 0
    if (!profile.is_explicit()) {
        Mlt::Profile ntscProfile("dv_ntsc");
        Mlt::Profile palProfile("dv_pal");
        if (ui->v4lWidthSpinBox->value() == ntscProfile.width() && ui->v4lHeightSpinBox->value() == ntscProfile.height()) {
            profile.set_sample_aspect(ntscProfile.sample_aspect_num(), ntscProfile.sample_aspect_den());
            profile.set_progressive(ntscProfile.progressive());
            profile.set_colorspace(ntscProfile.colorspace());
            profile.set_frame_rate(ntscProfile.frame_rate_num(), ntscProfile.frame_rate_den());
        } else if (ui->v4lWidthSpinBox->value() == palProfile.width() && ui->v4lHeightSpinBox->value() == palProfile.height()) {
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
    }
#endif
    Mlt::Producer* p = 0;
    if (ui->videoCombo->currentIndex() > 0) {
        p = new Mlt::Producer(profile, QString("dshow:video=%1")
                          .arg(ui->videoCombo->currentText())
                          .toLatin1().constData());
    }
    if (ui->audioCombo->currentIndex() > 0) {
        Mlt::Producer* audio = new Mlt::Producer(profile,
            QString("dshow:audio=%1").arg(ui->audioCombo->currentText())
                                     .toLatin1().constData());
        if (p && p->is_valid() && audio->is_valid()) {
            Mlt::Tractor* tractor = new Mlt::Tractor;
            tractor->set("_profile", profile.get_profile(), 0);
            tractor->set("resource", p->get("resource"));
            tractor->set("resource2", audio->get("resource"));
            tractor->set_track(*p, 0);
            delete p;
            tractor->set_track(*audio, 1);
            delete audio;
            p = tractor;
        } else {
            p = audio;
        }
    }
    if (!p || !p->is_valid()) {
        delete p;
        p = new Mlt::Producer(profile, "color:");
        if (ui->videoCombo->currentIndex() > 0) {
            p->set("resource", QString("dshow:video=%1")
                   .arg(ui->videoCombo->currentText())
                   .toLatin1().constData());
        }
        if (ui->audioCombo->currentIndex() > 0) {
            QString resource = QString("dshow:audio=%1").arg(ui->audioCombo->currentText());
            if (ui->videoCombo->currentIndex() > 0) {
                p->set("resource2", resource.toLatin1().constData());
            } else {
                p->set("resource", resource.toLatin1().constData());
            }
        }
        p->set("error", 1);
    }
    p->set("force_seekable", 0);
    return p;
}

void DirectShowVideoWidget::setProducer(Mlt::Producer *producer)
{
    QString resource = QString(producer->get("resource"));
    QString resource2 = QString(producer->get("resource2"));
    const char* videoDevice = "dshow:video=";
    const char* audioDevice = "dshow:audio=";
    if (resource.startsWith(videoDevice)) {
        QStringRef name = resource.midRef(qstrlen(videoDevice));
        for (int i = 1; i < ui->videoCombo->count(); i++) {
            if (ui->videoCombo->itemText(i) == name) {
                ui->videoCombo->setCurrentIndex(i);
                break;
            }
        }
    } else if (resource.startsWith(audioDevice)) {
        QStringRef name = resource.midRef(qstrlen(audioDevice));
        for (int i = 1; i < ui->audioCombo->count(); i++) {
            if (ui->audioCombo->itemText(i) == name) {
                ui->audioCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    if (resource2.startsWith(audioDevice)) {
        QStringRef name = resource2.midRef(qstrlen(audioDevice));
        for (int i = 1; i < ui->audioCombo->count(); i++) {
            if (ui->audioCombo->itemText(i) == name) {
                ui->audioCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    AbstractProducerWidget::setProducer(producer);
}

void DirectShowVideoWidget::on_videoCombo_activated(int index)
{
    Q_UNUSED(index)
    if (m_producer) {
        MLT.pause();
        delete m_producer;
        m_producer = new Mlt::Producer(producer(MLT.profile()));
        MLT.setProducer(m_producer);
        MLT.play();
        emit producerChanged(m_producer);
    }
}

void DirectShowVideoWidget::on_audioCombo_activated(int index)
{
    on_videoCombo_activated(index);
}
