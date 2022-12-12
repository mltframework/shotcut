/*
 * Copyright (c) 2014-2022 Meltytech, LLC
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
#include "shotcut_mlt_properties.h"
#include "settings.h"
#include <Logger.h>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>

DirectShowVideoWidget::DirectShowVideoWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DirectShowVideoWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label);
#ifdef Q_OS_WIN
    QFileInfo ffmpegPath(qApp->applicationDirPath(), "ffmpeg");
    QProcess proc;
    QStringList args;
    args << "-hide_banner" << "-list_devices" << "true" << "-f" << "dshow" << "-i" << "dummy";
    LOG_DEBUG() << ffmpegPath.absoluteFilePath() << args;
    proc.setStandardOutputFile(QProcess::nullDevice());
    proc.setReadChannel(QProcess::StandardError);
    proc.start(ffmpegPath.absoluteFilePath(), args, QIODevice::ReadOnly);
    bool started = proc.waitForStarted(2000);
    bool finished = false;
    QCoreApplication::processEvents();
    if (started) {
        finished = proc.waitForFinished(4000);
        QCoreApplication::processEvents();
    }

    bool isVideo = true;
    QString description;
    QString name;
    auto currentVideo = 1;
    auto currentAudio = 1;
    if (started && finished && proc.exitStatus() == QProcess::NormalExit) {
        QString output = proc.readAll();
        foreach (const QString &line, output.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts)) {
            auto i = line.indexOf("] \"");
            if (i > -1) {
                auto j = line.indexOf("\" (");
                if (j > -1) {
                    description = line.mid(i + 3, j - i - 3);
                    isVideo = line.mid(j + 3).startsWith("video");
                }
            } else {
                QString s("]   Alternative name \"");
                i = line.indexOf(s);
                if (i > -1) {
                    name = line.mid(i + s.size()).replace('\"', "");
                    LOG_DEBUG() << (isVideo ? "video" : "audio") << description << name;
                    if (isVideo) {
                        if (Settings.videoInput() == name) {
                            currentVideo = ui->videoCombo->count();
                        }
                        ui->videoCombo->addItem(description, name);
                    } else {
                        if (Settings.audioInput() == name) {
                            currentAudio = ui->audioCombo->count();
                        }
                        ui->audioCombo->addItem(description, name);
                    }
                }
            }
        }
    }

    if (ui->videoCombo->count() > 1)
        ui->videoCombo->setCurrentIndex(currentVideo);
    if (ui->audioCombo->count() > 1)
        ui->audioCombo->setCurrentIndex(currentAudio);
#endif
}

DirectShowVideoWidget::~DirectShowVideoWidget()
{
    delete ui;
}

Mlt::Producer *DirectShowVideoWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = 0;
    if (ui->videoCombo->currentIndex() > 0) {
        LOG_DEBUG() << ui->videoCombo->currentData().toString();
        p = new Mlt::Producer(profile, QString("dshow:video=%1")
                              .arg(ui->videoCombo->currentData().toString())
                              .toUtf8().constData());
    }
    if (ui->audioCombo->currentIndex() > 0) {
        Mlt::Producer *audio = new Mlt::Producer(profile,
                                                 QString("dshow:audio=%1").arg(ui->audioCombo->currentData().toString())
                                                 .toLatin1().constData());
        if (p && p->is_valid() && audio->is_valid()) {
            Mlt::Tractor *tractor = new Mlt::Tractor;
            tractor->set("_profile", profile.get_profile(), 0);
            tractor->set("resource1", p->get("resource"));
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
                   .arg(ui->videoCombo->currentData().toString())
                   .toUtf8().constData());
        }
        if (ui->audioCombo->currentIndex() > 0) {
            QString resource = QString("dshow:audio=%1").arg(ui->audioCombo->currentData().toString());
            if (ui->videoCombo->currentIndex() > 0) {
                p->set("resource2", resource.toUtf8().constData());
            } else {
                p->set("resource", resource.toUtf8().constData());
            }
        }
        p->set("error", 1);
    }
    p->set("force_seekable", 0);
    p->set(kBackgroundCaptureProperty, 1);
    p->set(kShotcutCaptionProperty, tr("Audio/Video Device").toUtf8().constData());
    if (ui->audioCombo->currentIndex() > 0) {
        Settings.setAudioInput(ui->audioCombo->currentData().toString());
    }
    if (ui->videoCombo->currentIndex() > 0) {
        Settings.setVideoInput(ui->videoCombo->currentData().toString());
    }
    return p;
}

void DirectShowVideoWidget::setProducer(Mlt::Producer *producer)
{
    QString resource = producer->get("resource1") ? QString(producer->get("resource1")) : QString(
                           producer->get("resource"));
    QString resource2 = QString(producer->get("resource2"));
    LOG_DEBUG() << "resource" << resource;
    LOG_DEBUG() << "resource2" << resource2;
    const char *videoDevice = "dshow:video=";
    const char *audioDevice = "dshow:audio=";
    ui->videoCombo->setCurrentIndex(0);
    ui->audioCombo->setCurrentIndex(0);
    if (resource.startsWith(videoDevice)) {
        auto name = resource.mid(qstrlen(videoDevice));
        for (int i = 1; i < ui->videoCombo->count(); i++) {
            if (ui->videoCombo->itemData(i).toString() == name) {
                ui->videoCombo->setCurrentIndex(i);
                break;
            }
        }
    } else if (resource.startsWith(audioDevice)) {
        auto name = resource.mid(qstrlen(audioDevice));
        for (int i = 1; i < ui->audioCombo->count(); i++) {
            if (ui->audioCombo->itemData(i).toString() == name) {
                ui->audioCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    if (resource2.startsWith(audioDevice)) {
        auto name = resource2.mid(qstrlen(audioDevice));
        for (int i = 1; i < ui->audioCombo->count(); i++) {
            if (ui->audioCombo->itemData(i).toString() == name) {
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
}

void DirectShowVideoWidget::on_audioCombo_activated(int index)
{
    on_videoCombo_activated(index);
}
