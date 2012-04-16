/*
 * Copyright (c) 2012 Meltytech, LLC
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

#include "avformatproducerwidget.h"
#include "ui_avformatproducerwidget.h"
#include "sdlwidget.h"
#include <QtDebug>
#include <QtGui/QComboBox>

AvformatProducerWidget::AvformatProducerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AvformatProducerWidget)
    , m_defaultDuration(-1)
{
    ui->setupUi(this);
    connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onFrameReceived(Mlt::QFrame)));
}

AvformatProducerWidget::~AvformatProducerWidget()
{
    delete ui;
}

Mlt::Producer* AvformatProducerWidget::producer(Mlt::Profile& profile)
{
    Mlt::Producer* p = new Mlt::Producer(profile, m_producer->get("resource"));
    return p;
}

void AvformatProducerWidget::reopen(Mlt::Producer* p)
{
    int length = ui->durationSpinBox->value();
    int out = m_producer->get_out();
    int position = m_producer->position();
    double speed = m_producer->get_speed();

    p->set("length", length);
    if (out + 1 >= m_producer->get_length())
        p->set("out", length - 1);
    else if (out >= length)
        p->set("out", length - 1);
    else
        p->set("out", out);
    if (position > p->get_out())
        position = p->get_out();
    p->set("in", m_producer->get_in());
    if (MLT.open(p)) {
        setProducer(0);
        return;
    }
    connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onFrameReceived(Mlt::QFrame)));
    emit producerReopened();
    emit producerChanged();
    MLT.seek(position);
    MLT.play(speed);
    setProducer(p);
}

void AvformatProducerWidget::onFrameReceived(Mlt::QFrame)
{
    disconnect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, 0);
    ui->tabWidget->setTabEnabled(0, false);
    ui->tabWidget->setTabEnabled(1, false);
    ui->tabWidget->setTabEnabled(2, false);
    if (m_defaultDuration == -1)
        m_defaultDuration = m_producer->get_length();

    QString s = QString::fromUtf8(m_producer->get("resource"));
    ui->filenameLabel->setText(ui->filenameLabel->fontMetrics().elidedText(s, Qt::ElideLeft, width() - 30));
    ui->notesTextEdit->setPlainText(QString::fromUtf8(m_producer->get("meta.attr.comment.markup")));
    ui->durationSpinBox->setValue(m_producer->get_length());

    // populate the track combos
    int n = m_producer->get_int("meta.media.nb_streams");
    int videoIndex = 1;
    int audioIndex = 1;
    bool populateTrackCombos = (ui->videoTrackComboBox->count() == 0 &&
                                ui->audioTrackComboBox->count() == 0);
    for (int i = 0; i < n; i++) {
        QString key = QString("meta.media.%1.stream.type").arg(i);
        QString streamType(m_producer->get(key.toAscii().constData()));
        if (streamType == "video") {
            key = QString("meta.media.%1.codec.name").arg(i);
            QString codec(m_producer->get(key.toAscii().constData()));
            key = QString("meta.media.%1.codec.width").arg(i);
            QString width(m_producer->get(key.toAscii().constData()));
            key = QString("meta.media.%1.codec.height").arg(i);
            QString height(m_producer->get(key.toAscii().constData()));
            QString name = QString("%1: %2x%3 %4")
                    .arg(videoIndex)
                    .arg(width)
                    .arg(height)
                    .arg(codec);
            if (populateTrackCombos) {
                if (ui->videoTrackComboBox->count() == 0)
                    ui->videoTrackComboBox->addItem(tr("None"), -1);
                ui->videoTrackComboBox->addItem(name, i);
            }
            if (i == m_producer->get_int("video_index")) {
                key = QString("meta.media.%1.codec.long_name").arg(i);
                QString codec(m_producer->get(key.toAscii().constData()));
                ui->videoTableWidget->setItem(0, 1, new QTableWidgetItem(codec));
                key = QString("meta.media.%1.codec.pix_fmt").arg(i);
                ui->videoTableWidget->setItem(3, 1, new QTableWidgetItem(
                    m_producer->get(key.toAscii().constData())));
                ui->videoTrackComboBox->setCurrentIndex(videoIndex);
            }
            ui->tabWidget->setTabEnabled(0, true);
            videoIndex++;
        }
        else if (streamType == "audio") {
            key = QString("meta.media.%1.codec.name").arg(i);
            QString codec(m_producer->get(key.toAscii().constData()));
            key = QString("meta.media.%1.codec.channels").arg(i);
            QString channels(m_producer->get(key.toAscii().constData()));
            key = QString("meta.media.%1.codec.sample_rate").arg(i);
            QString sampleRate(m_producer->get(key.toAscii().constData()));
            QString name = QString("%1: %2 ch %3 KHz %4")
                    .arg(audioIndex)
                    .arg(channels)
                    .arg(sampleRate.toDouble()/1000)
                    .arg(codec);
            if (populateTrackCombos) {
                if (ui->audioTrackComboBox->count() == 0)
                    ui->audioTrackComboBox->addItem(tr("None"), -1);
                ui->audioTrackComboBox->addItem(name, i);
            }
            if (i == m_producer->get_int("audio_index")) {
                key = QString("meta.media.%1.codec.long_name").arg(i);
                QString codec(m_producer->get(key.toAscii().constData()));
                ui->audioTableWidget->setItem(0, 1, new QTableWidgetItem(codec));
                ui->audioTableWidget->setItem(1, 1, new QTableWidgetItem(channels));
                ui->audioTableWidget->setItem(2, 1, new QTableWidgetItem(sampleRate));
                key = QString("meta.media.%1.codec.sample_fmt").arg(i);
                ui->audioTableWidget->setItem(3, 1, new QTableWidgetItem(
                    m_producer->get(key.toAscii().constData())));
                ui->audioTrackComboBox->setCurrentIndex(audioIndex);
            }
            ui->tabWidget->setTabEnabled(1, true);
            audioIndex++;
        }
    }
    if (!ui->tabWidget->isTabEnabled(0))
        ui->tabWidget->setCurrentIndex(1);

    int width = m_producer->get_int("meta.media.width");
    int height = m_producer->get_int("meta.media.height");
    ui->videoTableWidget->setItem(1, 1, new QTableWidgetItem(QString("%1x%2").arg(width).arg(height)));

    double sar = m_producer->get_double("meta.media.sample_aspect_num");
    if (m_producer->get_double("meta.media.sample_aspect_den") > 0)
        sar /= m_producer->get_double("meta.media.sample_aspect_den");
    if (m_producer->get("force_aspect_ratio"))
        sar = m_producer->get_double("force_aspect_ratio");
    int dar_numerator = width * sar;
    int dar_denominator = height;
    if (height > 0) {
        if (int(sar * width / height * 100) == 133) {
            dar_numerator = 4;
            dar_denominator = 3;
        }
        else if (int(sar * width / height * 100) == 177) {
            dar_numerator = 16;
            dar_denominator = 9;
        }
    }
    if (m_producer->get("shotcut_aspect_num"))
        dar_numerator = m_producer->get_int("shotcut_aspect_num");
    if (m_producer->get("shotcut_aspect_den"))
        dar_denominator = m_producer->get_int("shotcut_aspect_den");
    ui->aspectNumSpinBox->setValue(dar_numerator);
    ui->aspectDenSpinBox->setValue(dar_denominator);

    double fps = m_producer->get_double("meta.media.frame_rate_num");
    if (m_producer->get_double("meta.media.frame_rate_den") > 0)
        fps /= m_producer->get_double("meta.media.frame_rate_den");
    if (m_producer->get("force_fps"))
        fps = m_producer->get_double("fps");
    ui->videoTableWidget->setItem(2, 1, new QTableWidgetItem(QString::number(fps)));

    int progressive = m_producer->get_int("meta.media.progressive");
    if (m_producer->get("force_progressive"))
        progressive = m_producer->get_int("force_progressive");
    ui->scanComboBox->setCurrentIndex(progressive);

    int tff = m_producer->get_int("meta.media.top_field_first");
    if (m_producer->get("force_tff"))
        tff = m_producer->get_int("force_tff");
    ui->fieldOrderComboBox->setCurrentIndex(tff);
    ui->fieldOrderComboBox->setEnabled(!progressive);

    if (populateTrackCombos) {
        for (int i = 0; i < m_producer->count(); i++) {
            QString name(m_producer->get_name(i));
            if (name.startsWith("meta.attr.") && name.endsWith(".markup")) {
                int row = ui->metadataTable->rowCount();
                ui->metadataTable->setRowCount(row + 1);
                ui->metadataTable->setItem(row, 0, new QTableWidgetItem(name.section('.', -2, -2)));
                ui->metadataTable->setItem(row, 1, new QTableWidgetItem(m_producer->get(i)));
                ui->tabWidget->setTabEnabled(2, true);
            }
        }
    }
}

void AvformatProducerWidget::on_resetButton_clicked()
{
    Mlt::Producer* p = producer(MLT.profile());
    ui->durationSpinBox->setValue(m_defaultDuration);
    reopen(p);
    connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onFrameReceived(Mlt::QFrame)));
}

void AvformatProducerWidget::on_videoTrackComboBox_activated(int index)
{
    if (m_producer) {
        Mlt::Producer* p = producer(MLT.profile());
        p->set("video_index", ui->videoTrackComboBox->itemData(index).toInt());
        reopen(p);
        connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onFrameReceived(Mlt::QFrame)));
    }
}

void AvformatProducerWidget::on_audioTrackComboBox_activated(int index)
{
    if (m_producer) {
        Mlt::Producer* p = producer(MLT.profile());
        p->set("audio_index", ui->audioTrackComboBox->itemData(index).toInt());
        reopen(p);
        connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onFrameReceived(Mlt::QFrame)));
    }
}

void AvformatProducerWidget::on_scanComboBox_activated(int index)
{
    if (m_producer) {
        int progressive = m_producer->get_int("meta.media.progressive");
        ui->fieldOrderComboBox->setEnabled(!progressive);
        if (m_producer->get("force_progressive") || progressive != index)
            // We need to set these force_ properties as a string so they can be properly removed
            // by setting them NULL.
            m_producer->set("force_progressive", QString::number(index).toAscii().constData());
        emit producerChanged();
        connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onFrameReceived(Mlt::QFrame)));
    }
}

void AvformatProducerWidget::on_fieldOrderComboBox_activated(int index)
{
    if (m_producer) {
        int tff = m_producer->get_int("meta.media.top_field_first");
        if (m_producer->get("force_tff") || tff != index)
            m_producer->set("force_tff", QString::number(index).toAscii().constData());
        emit producerChanged();
        connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onFrameReceived(Mlt::QFrame)));
    }
}

void AvformatProducerWidget::on_aspectNumSpinBox_valueChanged(int)
{
    if (m_producer) {
        double new_sar = double(ui->aspectNumSpinBox->value() * m_producer->get_int("meta.media.height")) /
            double(ui->aspectDenSpinBox->value() * m_producer->get_int("meta.media.width"));
        double sar = m_producer->get_double("meta.media.sample_aspect_num");
        if (m_producer->get_double("meta.media.sample_aspect_den") > 0)
            sar /= m_producer->get_double("meta.media.sample_aspect_den");
        if (m_producer->get("force_aspect_ratio") || new_sar != sar) {
            m_producer->set("force_aspect_ratio", QString::number(new_sar).toAscii().constData());
            m_producer->set("shotcut_aspect_num", ui->aspectNumSpinBox->text().toAscii().constData());
            m_producer->set("shotcut_aspect_den", ui->aspectDenSpinBox->text().toAscii().constData());
        }
        emit producerChanged();
        connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onFrameReceived(Mlt::QFrame)));
    }
}

void AvformatProducerWidget::on_aspectDenSpinBox_valueChanged(int i)
{
    on_aspectNumSpinBox_valueChanged(i);
}

void AvformatProducerWidget::on_durationSpinBox_editingFinished()
{
    if (!m_producer)
        return;
    if (ui->durationSpinBox->value() == m_producer->get_length())
        return;
    Mlt::Producer* p = producer(MLT.profile());
    p->pass_list(*m_producer, "audio_index, video_index, force_aspect_ratio,"
                 "force_progressive, force_tff,"
                 "shotcut_aspect_num, shotcut_aspect_den");
    reopen(p);
}
