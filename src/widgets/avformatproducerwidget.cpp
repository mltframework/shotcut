/*
 * Copyright (c) 2012-2017 Meltytech, LLC
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
#include "util.h"
#include "mltcontroller.h"
#include "shotcut_mlt_properties.h"
#include "jobqueue.h"
#include "jobs/ffprobejob.h"
#include "jobs/ffmpegjob.h"
#include "settings.h"
#include "util.h"
#include "Logger.h"
#include <QtWidgets>

bool ProducerIsTimewarp( Mlt::Producer* producer )
{
    return QString::fromUtf8(producer->get("mlt_service")) == "timewarp";
}

char* GetFilenameFromProducer( Mlt::Producer* producer )
{
    char* resource = NULL;
    if (ProducerIsTimewarp(producer))
    {
        resource = producer->get("warp_resource");
    }
    else
    {
        resource = producer->get("resource");
    }
    return resource;
}

double GetSpeedFromProducer( Mlt::Producer* producer )
{
    double speed = 1.0;
    if (ProducerIsTimewarp(producer) )
    {
        speed = fabs(producer->get_double("warp_speed"));
    }
    return speed;
}

AvformatProducerWidget::AvformatProducerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AvformatProducerWidget)
    , m_defaultDuration(-1)
    , m_recalcDuration(true)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->filenameLabel);
    connect(MLT.videoWidget(), SIGNAL(frameDisplayed(const SharedFrame&)), this, SLOT(onFrameDisplayed(const SharedFrame&)));
}

AvformatProducerWidget::~AvformatProducerWidget()
{
    delete ui;
}

Mlt::Producer* AvformatProducerWidget::newProducer(Mlt::Profile& profile)
{
    Mlt::Producer* p = 0;
    if ( ui->speedSpinBox->value() == 1.0 )
    {
        p = new Mlt::Producer(profile, GetFilenameFromProducer(producer()));
    }
    else
    {
        double warpspeed = ui->speedSpinBox->value();
        char* filename = GetFilenameFromProducer(producer());
#ifdef Q_OS_MAC
        // On macOS MLT reads current locale as "C" regardless of what is in System Preferences.
        QString s = QString("%1:%2:%3").arg("timewarp").arg(warpspeed).arg(filename);
#else
        QString s = QString("%1:%L2:%3").arg("timewarp").arg(warpspeed).arg(filename);
#endif
        p = new Mlt::Producer(profile, s.toUtf8().constData());
        p->set(kShotcutProducerProperty, "avformat");
    }
    if (p->is_valid())
        p->set("video_delay", double(ui->syncSlider->value()) / 1000);
    return p;
}

void AvformatProducerWidget::keyPressEvent(QKeyEvent* event)
{
    if (ui->speedSpinBox->hasFocus() &&
            (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)) {
        ui->speedSpinBox->clearFocus();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void AvformatProducerWidget::reopen(Mlt::Producer* p)
{
    int length = ui->durationSpinBox->value();
    int out = m_producer->get_out();
    int position = m_producer->position();
    double speed = m_producer->get_speed();

    if( m_recalcDuration )
    {
        double oldSpeed = GetSpeedFromProducer(producer());
        double newSpeed = ui->speedSpinBox->value();
        double speedRatio = oldSpeed / newSpeed;
        int in = m_producer->get_in();

        length = qRound(length * speedRatio);
        p->set("length", length);
        p->set_in_and_out(qMin(qRound(in * speedRatio), length - 1),
                          qMin(qRound(out * speedRatio), length - 1));
        position = qRound(position * speedRatio);
    }
    else
    {
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
    }
    if (MLT.setProducer(p)) {
        AbstractProducerWidget::setProducer(0);
        return;
    }
    MLT.stop();
    connect(MLT.videoWidget(), SIGNAL(frameDisplayed(const SharedFrame&)), this, SLOT(onFrameDisplayed(const SharedFrame&)));
    emit producerReopened();
    emit producerChanged(p);
    MLT.seek(position);
    MLT.play(speed);
    setProducer(p);
}

void AvformatProducerWidget::recreateProducer()
{
    Mlt::Producer* p = newProducer(MLT.profile());
    p->pass_list(*m_producer, "audio_index, video_index, force_aspect_ratio,"
                 "video_delay, force_progressive, force_tff,"
                 kAspectRatioNumerator ","
                 kAspectRatioDenominator ","
                 kShotcutHashProperty ","
                 kPlaylistIndexProperty ","
                 kShotcutSkipConvertProperty);
    Mlt::Controller::copyFilters(*m_producer, *p);
    if (m_producer->get(kMultitrackItemProperty)) {
        emit producerChanged(p);
        delete p;
    } else {
        reopen(p);
    }
}

void AvformatProducerWidget::onFrameDisplayed(const SharedFrame&)
{
    // This forces avformat-novalidate or unloaded avformat to load and get
    // media information.
    delete m_producer->get_frame();

    int tabIndex = ui->tabWidget->currentIndex();
    ui->tabWidget->setTabEnabled(0, false);
    ui->tabWidget->setTabEnabled(1, false);
    ui->tabWidget->setTabEnabled(2, false);
    if (m_defaultDuration == -1)
        m_defaultDuration = m_producer->get_length();

    double warpSpeed = GetSpeedFromProducer(producer());
    QString resource = QString::fromUtf8(GetFilenameFromProducer(producer()));
    QString name = Util::baseName(resource);
    QString caption = name;
    if(warpSpeed != 1.0)
        caption = QString("%1 (%2x)").arg(name).arg(warpSpeed);
    m_producer->set(kShotcutCaptionProperty, caption.toUtf8().constData());
    m_producer->set(kShotcutDetailProperty, resource.toUtf8().constData());
    ui->filenameLabel->setText(ui->filenameLabel->fontMetrics().elidedText(caption, Qt::ElideLeft, width() - 30));
    ui->filenameLabel->setToolTip(resource);
    ui->notesTextEdit->setPlainText(QString::fromUtf8(m_producer->get(kCommentProperty)));
    ui->durationSpinBox->setValue(m_producer->get_length());
    m_recalcDuration = false;
    ui->speedSpinBox->setValue(warpSpeed);

    // populate the track combos
    int n = m_producer->get_int("meta.media.nb_streams");
    int videoIndex = 1;
    int audioIndex = 1;
    int totalAudioChannels = 0;
    bool populateTrackCombos = (ui->videoTrackComboBox->count() == 0 &&
                                ui->audioTrackComboBox->count() == 0);
    for (int i = 0; i < n; i++) {
        QString key = QString("meta.media.%1.stream.type").arg(i);
        QString streamType(m_producer->get(key.toLatin1().constData()));
        if (streamType == "video") {
            key = QString("meta.media.%1.codec.name").arg(i);
            QString codec(m_producer->get(key.toLatin1().constData()));
            key = QString("meta.media.%1.codec.width").arg(i);
            QString width(m_producer->get(key.toLatin1().constData()));
            key = QString("meta.media.%1.codec.height").arg(i);
            QString height(m_producer->get(key.toLatin1().constData()));
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
                QString codec(m_producer->get(key.toLatin1().constData()));
                ui->videoTableWidget->setItem(0, 1, new QTableWidgetItem(codec));
                key = QString("meta.media.%1.codec.pix_fmt").arg(i);
                ui->videoTableWidget->setItem(3, 1, new QTableWidgetItem(
                    m_producer->get(key.toLatin1().constData())));
                ui->videoTrackComboBox->setCurrentIndex(videoIndex);
            }
            ui->tabWidget->setTabEnabled(0, true);
            videoIndex++;
        }
        else if (streamType == "audio") {
            key = QString("meta.media.%1.codec.name").arg(i);
            QString codec(m_producer->get(key.toLatin1().constData()));
            key = QString("meta.media.%1.codec.channels").arg(i);
            QString channels(m_producer->get(key.toLatin1().constData()));
            totalAudioChannels += channels.toInt();
            key = QString("meta.media.%1.codec.sample_rate").arg(i);
            QString sampleRate(m_producer->get(key.toLatin1().constData()));
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
            if ( QString::number(i) == m_producer->get("audio_index")) {
                key = QString("meta.media.%1.codec.long_name").arg(i);
                QString codec(m_producer->get(key.toLatin1().constData()));
                ui->audioTableWidget->setItem(0, 1, new QTableWidgetItem(codec));
                ui->audioTableWidget->setItem(1, 1, new QTableWidgetItem(channels));
                ui->audioTableWidget->setItem(2, 1, new QTableWidgetItem(sampleRate));
                key = QString("meta.media.%1.codec.sample_fmt").arg(i);
                ui->audioTableWidget->setItem(3, 1, new QTableWidgetItem(
                    m_producer->get(key.toLatin1().constData())));
                ui->audioTrackComboBox->setCurrentIndex(audioIndex);
            }
            ui->tabWidget->setTabEnabled(1, true);
            audioIndex++;
        }
    }
    if (populateTrackCombos && ui->audioTrackComboBox->count() > 2)
        ui->audioTrackComboBox->addItem(tr("All"), "all");

    if (m_producer->get("audio_index") == QString("-1")) {
        ui->audioTrackComboBox->setCurrentIndex(0);
        ui->audioTableWidget->setItem(0, 1, new QTableWidgetItem(""));
        ui->audioTableWidget->setItem(1, 1, new QTableWidgetItem("0"));
        ui->audioTableWidget->setItem(2, 1, new QTableWidgetItem(""));
        ui->audioTableWidget->setItem(3, 1, new QTableWidgetItem(""));
    }
    else if (m_producer->get("audio_index") == QString("all")) {
        ui->audioTrackComboBox->setCurrentIndex(ui->audioTrackComboBox->count()-1);
        ui->audioTableWidget->setItem(0, 1, new QTableWidgetItem(""));
        ui->audioTableWidget->setItem(1, 1, new QTableWidgetItem(QString::number(totalAudioChannels)));
        ui->audioTableWidget->setItem(2, 1, new QTableWidgetItem(""));
        ui->audioTableWidget->setItem(3, 1, new QTableWidgetItem(""));
    }
    if (m_producer->get("video_index") == QString("-1")) {
        ui->videoTrackComboBox->setCurrentIndex(0);
        ui->videoTableWidget->setItem(0, 1, new QTableWidgetItem(""));
        ui->videoTableWidget->setItem(1, 1, new QTableWidgetItem(""));
        ui->videoTableWidget->setItem(2, 1, new QTableWidgetItem(""));
        ui->videoTableWidget->setItem(3, 1, new QTableWidgetItem(""));
    }

    // Restore the previous tab, or select the first enabled tab.
    if (ui->tabWidget->isTabEnabled(tabIndex))
        ui->tabWidget->setCurrentIndex(tabIndex);
    else if (ui->tabWidget->isTabEnabled(0))
        ui->tabWidget->setCurrentIndex(0);
    else if (ui->tabWidget->isTabEnabled(1))
        ui->tabWidget->setCurrentIndex(1);

    int width = m_producer->get_int("meta.media.width");
    int height = m_producer->get_int("meta.media.height");
    if (width || height)
        ui->videoTableWidget->setItem(1, 1, new QTableWidgetItem(QString("%1x%2").arg(width).arg(height)));

    // We can stop listening to this signal if this is audio-only or if we have
    // received the video resolution.
    if (videoIndex == 1 || width || height)
        disconnect(MLT.videoWidget(), SIGNAL(frameDisplayed(const SharedFrame&)), this, 0);

    double sar = m_producer->get_double("meta.media.sample_aspect_num");
    if (m_producer->get_double("meta.media.sample_aspect_den") > 0)
        sar /= m_producer->get_double("meta.media.sample_aspect_den");
    if (m_producer->get("force_aspect_ratio"))
        sar = m_producer->get_double("force_aspect_ratio");
    int dar_numerator = width * sar;
    int dar_denominator = height;
    if (height > 0) {
        switch (int(sar * width / height * 100)) {
        case 133:
            dar_numerator = 4;
            dar_denominator = 3;
            break;
        case 177:
            dar_numerator = 16;
            dar_denominator = 9;
            break;
        case 56:
            dar_numerator = 9;
            dar_denominator = 16;
        }
    }
    if (m_producer->get(kAspectRatioNumerator))
        dar_numerator = m_producer->get_int(kAspectRatioNumerator);
    if (m_producer->get(kAspectRatioDenominator))
        dar_denominator = m_producer->get_int(kAspectRatioDenominator);
    ui->aspectNumSpinBox->blockSignals(true);
    ui->aspectNumSpinBox->setValue(dar_numerator);
    ui->aspectNumSpinBox->blockSignals(false);
    ui->aspectDenSpinBox->blockSignals(true);
    ui->aspectDenSpinBox->setValue(dar_denominator);
    ui->aspectDenSpinBox->blockSignals(false);

    double fps = m_producer->get_double("meta.media.frame_rate_num");
    if (m_producer->get_double("meta.media.frame_rate_den") > 0)
        fps /= m_producer->get_double("meta.media.frame_rate_den");
    if (m_producer->get("force_fps"))
        fps = m_producer->get_double("fps");
    bool isVariableFrameRate = m_producer->get_int("meta.media.variable_frame_rate");
    if (fps != 0.0 ) {
        ui->videoTableWidget->setItem(2, 1, new QTableWidgetItem(QString("%L1 %2").arg(fps)
                                      .arg(isVariableFrameRate? tr("(variable)") : "")));
    }

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
    ui->syncSlider->setValue(qRound(m_producer->get_double("video_delay") * 1000.0));

    if (Settings.showConvertClipDialog()
            && !m_producer->get_int(kShotcutSkipConvertProperty)
            && !m_producer->get_int(kPlaylistIndexProperty)
            && !m_producer->get(kMultitrackItemProperty)) {
        m_producer->set(kShotcutSkipConvertProperty, true);
        if (isVariableFrameRate) {
            MLT.pause();
            LOG_INFO() << resource << "is variable frame rate";
            TranscodeDialog dialog(tr("This file is variable frame rate, which is not reliable for editing. "
                                      "Do you want to convert it to an edit-friendly format?\n\n"
                                      "If yes, choose a format below and then click OK to choose a file name. "
                                      "After choosing a file name, a job is created. "
                                      "When it is done, double-click the job to open it.\n"), this);
            dialog.showCheckBox();
            convert(dialog);
        }
        if (QFile::exists(resource) && !MLT.isSeekable(m_producer.data())) {
            MLT.pause();
            LOG_INFO() << resource << "is not seekable";
            TranscodeDialog dialog(tr("This file does not support seeking and cannot be used for editing. "
                                      "Do you want to convert it to an edit-friendly format?\n\n"
                                      "If yes, choose a format below and then click OK to choose a file name. "
                                      "After choosing a file name, a job is created. "
                                      "When it is done, double-click the job to open it.\n"), this);
            dialog.showCheckBox();
            convert(dialog);
        }
    }
}

void AvformatProducerWidget::on_resetButton_clicked()
{
    ui->speedSpinBox->setValue(1.0);
    Mlt::Producer* p = newProducer(MLT.profile());
    ui->durationSpinBox->setValue(m_defaultDuration);
    ui->syncSlider->setValue(0);
    Mlt::Controller::copyFilters(*m_producer, *p);
    if (m_producer->get(kMultitrackItemProperty)) {
        emit producerChanged(p);
        delete p;
    } else {
        reopen(p);
    }
}

void AvformatProducerWidget::on_videoTrackComboBox_activated(int index)
{
    if (m_producer) {
        m_producer->set("video_index", ui->videoTrackComboBox->itemData(index).toInt());
        recreateProducer();
    }
}

void AvformatProducerWidget::on_audioTrackComboBox_activated(int index)
{
    if (m_producer) {
        m_producer->set("audio_index", ui->audioTrackComboBox->itemData(index).toString().toUtf8().constData());
        recreateProducer();
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
            m_producer->set("force_progressive", QString::number(index).toLatin1().constData());
        emit producerChanged(producer());
        connect(MLT.videoWidget(), SIGNAL(frameDisplayed(const SharedFrame&)), this, SLOT(onFrameDisplayed(const SharedFrame&)));
    }
}

void AvformatProducerWidget::on_fieldOrderComboBox_activated(int index)
{
    if (m_producer) {
        int tff = m_producer->get_int("meta.media.top_field_first");
        if (m_producer->get("force_tff") || tff != index)
            m_producer->set("force_tff", QString::number(index).toLatin1().constData());
        emit producerChanged(producer());
        connect(MLT.videoWidget(), SIGNAL(frameDisplayed(const SharedFrame&)), this, SLOT(onFrameDisplayed(const SharedFrame&)));
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
            m_producer->set("force_aspect_ratio", QString::number(new_sar).toLatin1().constData());
            m_producer->set(kAspectRatioNumerator, ui->aspectNumSpinBox->text().toLatin1().constData());
            m_producer->set(kAspectRatioDenominator, ui->aspectDenSpinBox->text().toLatin1().constData());
        }
        emit producerChanged(producer());
        connect(MLT.videoWidget(), SIGNAL(frameDisplayed(const SharedFrame&)), this, SLOT(onFrameDisplayed(const SharedFrame&)));
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
    recreateProducer();
}


void AvformatProducerWidget::on_speedSpinBox_editingFinished()
{
    if (!m_producer)
        return;
    m_recalcDuration = true;
    recreateProducer();
}

void AvformatProducerWidget::on_syncSlider_valueChanged(int value)
{
    if (m_producer)
        m_producer->set("video_delay", double(value) / 1000);
}

void AvformatProducerWidget::on_actionOpenFolder_triggered()
{
    Util::showInFolder(GetFilenameFromProducer(producer()));
}

void AvformatProducerWidget::on_menuButton_clicked()
{
    QMenu menu;
    if (!MLT.resource().contains("://")) // not a network stream
        menu.addAction(ui->actionOpenFolder);
    menu.addAction(ui->actionCopyFullFilePath);
    menu.addAction(ui->actionFFmpegInfo);
    menu.addAction(ui->actionFFmpegIntegrityCheck);
    menu.addAction(ui->actionFFmpegConvert);
    menu.exec(ui->menuButton->mapToGlobal(QPoint(0, 0)));
}

void AvformatProducerWidget::on_actionCopyFullFilePath_triggered()
{
    qApp->clipboard()->setText(GetFilenameFromProducer(producer()));
}

void AvformatProducerWidget::on_notesTextEdit_textChanged()
{
    m_producer->set(kCommentProperty, ui->notesTextEdit->toPlainText().toUtf8().constData());
}

void AvformatProducerWidget::on_actionFFmpegInfo_triggered()
{
    QStringList args;
    args << "-v" << "quiet";
    args << "-print_format" << "ini";
    args << "-pretty";
    args << "-show_format" << "-show_programs" << "-show_streams";
    args << GetFilenameFromProducer(producer());
    AbstractJob* job = new FfprobeJob(args.last(), args);
    job->start();
}

void AvformatProducerWidget::on_actionFFmpegIntegrityCheck_triggered()
{
    QString resource = GetFilenameFromProducer(producer());
    QStringList args;
    args << "-xerror";
    args << "-err_detect" << "+explode";
    args << "-v" << "info";
    args << "-i" << resource;
    args << "-f" << "null" << "pipe:";
    JOBS.add(new FfmpegJob(resource, args));
}

void AvformatProducerWidget::on_actionFFmpegConvert_triggered()
{
    TranscodeDialog dialog(tr("Choose an edit-friendly format below and then click OK to choose a file name. "
                              "After choosing a file name, a job is created. "
                              "When it is done, double-click the job to open it.\n"), this);
    convert(dialog);
}

void AvformatProducerWidget::convert(TranscodeDialog& dialog)
{
    int result = dialog.exec();
    if (dialog.isCheckBoxChecked()) {
        Settings.setShowConvertClipDialog(false);
    }
    if (result == QDialog::Accepted) {
        QString resource = QString::fromUtf8(GetFilenameFromProducer(producer()));
        QString path = Settings.savePath();
        QStringList args;

        args << "-loglevel" << "verbose";
        args << "-i" << resource;
        // transcode all streams
        args << "-map" << "0";
        // except data, subtitles, and attachments
        args << "-map" << "-0:d" << "-map" << "-0:s" << "-map" << "-0:t" << "-ignore_unknown";

        switch (dialog.format()) {
        case 0:
            path.append("/%1.mp4");
            args << "-f" << "mp4" << "-codec:a" << "aac" << "-b:a" << "512k" << "-codec:v" << "libx264";
            args << "-preset" << "medium" << "-g" << "1" << "-crf" << "11";
            break;
        case 1:
            args << "-f" << "mov" << "-codec:a" << "alac" << "-codec:v" << "prores_ks" << "-profile:v" << "standard";
            path.append("/%1.mov");
            break;
        case 2:
            args << "-f" << "matroska" << "-codec:a" << "flac" << "-codec:v" << "ffv1" << "-coder" << "1";
            args << "-context" << "1" << "-g" << "1" << "-threads" << QString::number(QThread::idealThreadCount());
            path.append("/%1.mkv");
            break;
        }
        QFileInfo fi(resource);
        path = path.arg(fi.baseName());
        QString filename = QFileDialog::getSaveFileName(this, dialog.windowTitle(), path);
        if (!filename.isEmpty()) {
            if (filename == QDir::toNativeSeparators(resource)) {
                QMessageBox::warning(this, dialog.windowTitle(),
                                     QObject::tr("Unable to write file %1\n"
                                        "Perhaps you do not have permission.\n"
                                        "Try again with a different folder.")
                                     .arg(fi.fileName()));
                return;
            }
            if (Util::warnIfNotWritable(filename, this, dialog.windowTitle()))
                return;

            Settings.setSavePath(QFileInfo(filename).path());
            args << filename;
            JOBS.add(new FfmpegJob(filename, args, false));
        }
    }
}
