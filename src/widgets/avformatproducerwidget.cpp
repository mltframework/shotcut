/*
 * Copyright (c) 2012-2023 Meltytech, LLC
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
#include "dialogs/filedatedialog.h"
#include "jobqueue.h"
#include "jobs/ffprobejob.h"
#include "jobs/ffmpegjob.h"
#include "jobs/meltjob.h"
#include "jobs/postjobaction.h"
#include "jobs/gopro2gpxjob.h"
#include "settings.h"
#include "mainwindow.h"
#include "Logger.h"
#include "qmltypes/qmlapplication.h"
#include "proxymanager.h"
#include "dialogs/longuitask.h"
#include "spatialmedia/spatialmedia.h"

#include <QtWidgets>

static const auto kHandleSeconds = 15.0;

AvformatProducerWidget::AvformatProducerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AvformatProducerWidget)
    , m_defaultDuration(-1)
    , m_recalcDuration(true)
{
    ui->setupUi(this);
    ui->timelineDurationText->setFixedWidth(ui->durationSpinBox->width());
    ui->filenameLabel->setFrame(true);
    Util::setColorsToHighlight(ui->filenameLabel, QPalette::Base);
    connect(ui->applySpeedButton, SIGNAL(clicked()), SLOT(on_speedSpinBox_editingFinished()));
    connect(this, SIGNAL(producerChanged(Mlt::Producer *)), SLOT(onProducerChanged(Mlt::Producer *)));
}

AvformatProducerWidget::~AvformatProducerWidget()
{
    delete ui;
}

Mlt::Producer *AvformatProducerWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = 0;
    if ( ui->speedSpinBox->value() == 1.0 ) {
        p = new Mlt::Chain(profile, Util::GetFilenameFromProducer(producer(), false).toUtf8().constData());
    } else {
        // If the system language's numeric format and region's numeric format differ, then MLT
        // uses the language's numeric format while Qt is using the region's. Thus, to
        // supply a proper numeric format in string form to MLT, we must use MLT instead of
        // letting Qt convert it.
        Mlt::Properties tempProps;
        tempProps.set("speed", ui->speedSpinBox->value());
        QString warpspeed = QString::fromLatin1(tempProps.get("speed"));

        QString filename = Util::GetFilenameFromProducer(producer(), false);
        QString s = QString("%1:%2:%3").arg("timewarp").arg(warpspeed).arg(filename);
        p = new Mlt::Producer(profile, s.toUtf8().constData());
        p->set(kShotcutProducerProperty, "avformat");
    }
    if (p->is_valid()) {
        p->set("video_delay", double(ui->syncSlider->value()) / 1000);
        if (ui->pitchCheckBox->checkState() == Qt::Checked) {
            m_producer->set("warp_pitch", 1);
        }
    }
    return p;
}

void AvformatProducerWidget::setProducer(Mlt::Producer *p)
{
    AbstractProducerWidget::setProducer(p);
    emit producerChanged(p);
}

void AvformatProducerWidget::updateDuration()
{
    if (m_producer->get(kFilterInProperty) && m_producer->get(kFilterOutProperty)) {
        auto duration = m_producer->get_int(kFilterOutProperty) - m_producer->get_int(
                            kFilterInProperty) + 1;
        ui->timelineDurationLabel->show();
        ui->timelineDurationText->setText(m_producer->frames_to_time(duration));
        ui->timelineDurationText->show();
    } else {
        ui->timelineDurationLabel->hide();
        ui->timelineDurationLabel->setText(QString());
        ui->timelineDurationText->hide();
    }
}

void AvformatProducerWidget::rename()
{
    ui->filenameLabel->setFocus();
    ui->filenameLabel->selectAll();
}

void AvformatProducerWidget::offerConvert(QString message, bool set709Convert, bool setSubClip)
{
    m_producer->set(kShotcutSkipConvertProperty, true);
    LongUiTask::cancel();
    MLT.pause();
    TranscodeDialog dialog(message.append(
                               tr(" Do you want to convert it to an edit-friendly format?\n\n"
                                  "If yes, choose a format below and then click OK to choose a file name. "
                                  "After choosing a file name, a job is created. "
                                  "When it is done, it automatically replaces clips, or you can double-click the job to open it.\n")),
                           ui->scanComboBox->currentIndex(), this);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (!setSubClip) {
        dialog.showCheckBox();
    }
    dialog.set709Convert(set709Convert);
    dialog.showSubClipCheckBox();
    LOG_DEBUG() << "in" << m_producer->get_in() << "out" << m_producer->get_out() << "length" <<
                m_producer->get_length() - 1;
    dialog.setSubClipChecked(setSubClip && (m_producer->get_in() > 0
                                            || m_producer->get_out() < m_producer->get_length() - 1));
    convert(dialog);
}

void AvformatProducerWidget::keyPressEvent(QKeyEvent *event)
{
    if (ui->speedSpinBox->hasFocus() &&
            (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)) {
        ui->speedSpinBox->clearFocus();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void AvformatProducerWidget::onProducerChanged(Mlt::Producer *producer)
{
    if ( producer->get_producer() == m_producer->get_producer() ) {
        reloadProducerValues();
    }
}

void AvformatProducerWidget::reopen(Mlt::Producer *p)
{
    int length = ui->durationSpinBox->value();
    int out = m_producer->get_out();
    int position = m_producer->position();
    double speed = m_producer->get_speed();

    if ( m_recalcDuration ) {
        double oldSpeed = Util::GetSpeedFromProducer(producer());
        double newSpeed = ui->speedSpinBox->value();
        double speedRatio = oldSpeed / newSpeed;
        int in = m_producer->get_in();

        length = qRound(length * speedRatio);
        in = qMin(qRound(in * speedRatio), length - 1);
        out = qMin(qRound(out * speedRatio), length - 1);
        p->set("length", p->frames_to_time(length, mlt_time_clock));
        p->set_in_and_out(in, out);
        position = qRound(position * speedRatio);

        // Adjust filters.
        int n = p->filter_count();
        for (int j = 0; j < n; j++) {
            QScopedPointer<Mlt::Filter> filter(p->filter(j));
            if (filter && filter->is_valid() && !filter->get_int("_loader")) {
                in = qMin(qRound(filter->get_in() * speedRatio), length - 1);
                out = qMin(qRound(filter->get_out() * speedRatio), length - 1);
                filter->set_in_and_out(in, out);
                //TODO: keyframes
            }
        }
    } else {
        p->set("length", p->frames_to_time(length, mlt_time_clock));
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
    MLT.stop();
    if (MLT.setProducer(p)) {
        AbstractProducerWidget::setProducer(0);
        return;
    }
    emit producerReopened(false);
    emit producerChanged(p);
    MLT.seek(position);
    MLT.play(speed);
    setProducer(p);
}

void AvformatProducerWidget::recreateProducer()
{
    Mlt::Producer *p = newProducer(MLT.profile());
    Util::passProducerProperties(m_producer.data(), p);
    Util::updateCaption(p);
    Mlt::Controller::copyFilters(*m_producer, *p);
    if (m_producer->get(kMultitrackItemProperty)) {
        int length = ui->durationSpinBox->value();
        int in = m_producer->get_in();
        int out = m_producer->get_out();
        double oldSpeed = Util::GetSpeedFromProducer(producer());
        double newSpeed = ui->speedSpinBox->value();
        double speedRatio = oldSpeed / newSpeed;
        length = qRound(length * speedRatio);
        in = qMin(qRound(in * speedRatio), length - 1);
        out = qMin(qRound(out * speedRatio), length - 1);
        p->set("length", p->frames_to_time(length, mlt_time_clock));
        p->set_in_and_out(in, out);
        emit producerChanged(p);
        delete p;
    } else {
        reopen(p);
    }
}

void AvformatProducerWidget::reloadProducerValues()
{
    m_producer->probe();
    int tabIndex = ui->tabWidget->currentIndex();
    ui->tabWidget->setTabEnabled(0, false);
    ui->tabWidget->setTabEnabled(1, false);
    ui->tabWidget->setTabEnabled(2, false);
    if (m_defaultDuration == -1)
        m_defaultDuration = m_producer->get_length();

    double warpSpeed = Util::GetSpeedFromProducer(producer());
    QString resource = Util::GetFilenameFromProducer(producer());
    QString caption = Util::updateCaption(m_producer.data());
    ui->filenameLabel->setText(caption);
    ui->filenameLabel->setCursorPosition(caption.length());
    ui->filenameLabel->setToolTip(resource);
    ui->notesTextEdit->setPlainText(QString::fromUtf8(m_producer->get(kCommentProperty)));
    ui->durationSpinBox->setValue(m_producer->get_length());
    updateDuration();
    m_recalcDuration = false;
    ui->speedSpinBox->setValue(warpSpeed);
    if (warpSpeed == 1.0) {
        ui->pitchCheckBox->setEnabled(false);
    } else {
        ui->pitchCheckBox->setEnabled(true);
    }
    if (m_producer->get_int("warp_pitch") == 1) {
        ui->pitchCheckBox->setCheckState(Qt::Checked);
    } else {
        ui->pitchCheckBox->setCheckState(Qt::Unchecked);
    }
    ui->rangeComboBox->setEnabled(true);

    // Disable all actions if the file does not exist
    auto exists = QFile::exists(resource);
    ui->speedSpinBox->setEnabled(exists);
    ui->speedComboBox->setEnabled(exists);
    ui->applySpeedButton->setEnabled(exists);
    ui->durationSpinBox->setEnabled(exists);
    ui->menuButton->setEnabled(exists);
    ui->convertButton->setEnabled(exists);
    ui->reverseButton->setEnabled(exists);
    ui->proxyButton->setEnabled(exists);

    // populate the track combos
    int n = m_producer->get_int("meta.media.nb_streams");
    int videoIndex = 1;
    int audioIndex = 1;
    int totalAudioChannels = 0;
    bool populateTrackCombos = (ui->videoTrackComboBox->count() == 0 &&
                                ui->audioTrackComboBox->count() == 0);
    int color_range = !qstrcmp(m_producer->get("meta.media.color_range"), "full");

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
                QString pix_fmt = QString::fromLatin1(m_producer->get(key.toLatin1().constData()));
                if (pix_fmt.startsWith("yuvj")) {
                    color_range = 1;
                } else if (pix_fmt.contains("gbr") || pix_fmt.contains("rgb")) {
                    color_range = 1;
                    ui->rangeComboBox->setEnabled(false);
                }
                key = QString("meta.media.%1.codec.rotate").arg(i);
                int rotation = m_producer->property_exists("rotate") ?
                               m_producer->get_int("rotate") :
                               m_producer->get_int(key.toLatin1().constData());
                ui->rotationComboBox->setCurrentIndex(rotation / 90);
                ui->videoTableWidget->setItem(3, 1, new QTableWidgetItem(pix_fmt));
                key = QString("meta.media.%1.codec.colorspace").arg(i);
                int colorspace = m_producer->get_int(key.toLatin1().constData());
                QString csString = tr("unknown (%1)").arg(colorspace);
                switch (colorspace) {
                case 240:
                    csString = "SMPTE ST240";
                    break;
                case 601:
                    csString = "ITU-R BT.601";
                    break;
                case 709:
                    csString = "ITU-R BT.709";
                    break;
                case 9:
                case 10:
                    csString = "ITU-R BT.2020";
                    break;
                }
                ui->videoTableWidget->setItem(4, 1, new QTableWidgetItem(csString));
                key = QString("meta.media.%1.codec.color_trc").arg(i);
                int trc = m_producer->get_int(key.toLatin1().constData());
                QString trcString = tr("unknown (%1)").arg(trc);
                switch (trc) {
                case 0:
                    trcString = tr("NA");
                    break;
                case 1:
                    trcString = "ITU-R BT.709";
                    break;
                case 6:
                    trcString = "ITU-R BT.601";
                    break;
                case 7:
                    trcString = "SMPTE ST240";
                    break;
                case 11:
                    trcString = "IEC 61966-2-4";
                    break;
                case 14:
                    trcString = "ITU-R BT.2020";
                    break;
                case 15:
                    trcString = "ITU-R BT.2020";
                    break;
                case 16:
                    trcString = "SMPTE ST2084 (PQ)";
                    break;
                case 17:
                    trcString = "SMPTE ST428";
                    break;
                case 18:
                    trcString = "ARIB B67 (HLG)";
                    break;
                }
                QTableWidgetItem *trcItem = new QTableWidgetItem(trcString);
                trcItem->setData(Qt::UserRole, QVariant(trc));
                ui->videoTableWidget->setItem(5, 1, trcItem);
                ui->videoTrackComboBox->setCurrentIndex(videoIndex);
            }
            ui->tabWidget->setTabEnabled(0, true);
            videoIndex++;
        } else if (streamType == "audio") {
            key = QString("meta.media.%1.codec.name").arg(i);
            QString codec(m_producer->get(key.toLatin1().constData()));
            key = QString("meta.media.%1.codec.channels").arg(i);
            int channels(m_producer->get_int(key.toLatin1().constData()));
            totalAudioChannels += channels;
            key = QString("meta.media.%1.codec.sample_rate").arg(i);
            QString sampleRate(m_producer->get(key.toLatin1().constData()));
            QString name = QString("%1: %2 ch %3 KHz %4")
                           .arg(audioIndex)
                           .arg(channels)
                           .arg(sampleRate.toDouble() / 1000)
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
                const char *layout = mlt_audio_channel_layout_name(mlt_audio_channel_layout_default(channels));
                QString channelsStr = QString("%1 (%2)").arg(channels).arg(layout);
                ui->audioTableWidget->setItem(1, 1, new QTableWidgetItem(channelsStr));
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
    } else if (m_producer->get("audio_index") == QString("all")) {
        ui->audioTrackComboBox->setCurrentIndex(ui->audioTrackComboBox->count() - 1);
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
        ui->videoTableWidget->setItem(4, 1, new QTableWidgetItem(""));
        ui->videoTableWidget->setItem(5, 1, new QTableWidgetItem(""));
        ui->proxyButton->hide();
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
    if (width || height) {
        bool isProxy = m_producer->get_int(kIsProxyProperty) && m_producer->get(kOriginalResourceProperty);
        ui->videoTableWidget->setItem(1, 1, new QTableWidgetItem(QString("%1x%2 %3").arg(width).arg(height)
                                                                 .arg(isProxy ? tr("(PROXY)") : "")));
    }

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
        ui->videoTableWidget->setItem(2, 1, new QTableWidgetItem(QString("%L1 %2").arg(fps, 0, 'f', 6)
                                                                 .arg(isVariableFrameRate ? tr("(variable)") : "")));
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
    if (m_producer->get("color_range"))
        color_range = m_producer->get_int("color_range") == 2;
    else if (m_producer->get("force_full_range"))
        color_range = m_producer->get_int("force_full_range");
    ui->rangeComboBox->setCurrentIndex(color_range);

    if (populateTrackCombos) {
        for (int i = 0; i < m_producer->count(); i++) {
            QString name(m_producer->get_name(i));
            if (name.startsWith("meta.attr.") && name.endsWith(".markup")) {
                int row = ui->metadataTable->rowCount();
                ui->metadataTable->setRowCount(row + 1);
                ui->metadataTable->setItem(row, 0, new QTableWidgetItem(name.section('.', -2, -2)));
                ui->metadataTable->setItem(row, 1, new QTableWidgetItem(m_producer->get(i)));
                if (ui->metadataTable->item(row, 0)->text() == "handler_name"
                        && QString(m_producer->get(i)).contains("GoPro")) {
                    ui->actionExportGPX->setEnabled(true);
                }
                ui->tabWidget->setTabEnabled(2, true);
            }
        }
    }
    ui->syncSlider->setValue(qRound(m_producer->get_double("video_delay") * 1000.0));
    setSyncVisibility();

    if (Settings.showConvertClipDialog() && !m_producer->get_int(kShotcutSkipConvertProperty)) {
        auto transferItem = ui->videoTableWidget->item(5, 1);
        if (transferItem) LOG_INFO() << "color transfer" << transferItem->data(
                                             Qt::UserRole).toInt() << "=" << transferItem->text();
        if (transferItem && transferItem->data(Qt::UserRole).toInt() > 7
                && transferItem->data(Qt::UserRole).toInt() != 11
                && transferItem->data(Qt::UserRole).toInt() != 18) {
            // Transfer characteristics > SMPTE240M Probably need conversion except IEC61966-2-4 is OK
            QString trcString = ui->videoTableWidget->item(5, 1)->text();
            LOG_INFO() << resource << "Probable HDR" << trcString;
            offerConvert(
                tr("This file uses color transfer characteristics %1, which may result in incorrect colors or brightness in Shotcut.").arg(
                    trcString), true);
        } else if (isVariableFrameRate) {
            LOG_INFO() << resource << "is variable frame rate";
            offerConvert(tr("This file is variable frame rate, which is not reliable for editing."));
        } else if (QFile::exists(resource) && !MLT.isSeekable(m_producer.data())) {
            LOG_INFO() << resource << "is not seekable";
            offerConvert(tr("This file does not support seeking and cannot be used for editing."));
        } else if (QFile::exists(resource) && resource.endsWith(".m2t")) {
            LOG_INFO() << resource << "is HDV";
            offerConvert(tr("This file format (HDV) is not reliable for editing."));
        }
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
        // Save the default audio index for AudioLevelsTask.
        if (!m_producer->get(kDefaultAudioIndexProperty)) {
            m_producer->set(kDefaultAudioIndexProperty, m_producer->get_int("audio_index"));
        }
        m_producer->set("audio_index",
                        ui->audioTrackComboBox->itemData(index).toString().toUtf8().constData());
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
    }
}

void AvformatProducerWidget::on_fieldOrderComboBox_activated(int index)
{
    if (m_producer) {
        int tff = m_producer->get_int("meta.media.top_field_first");
        if (m_producer->get("force_tff") || tff != index)
            m_producer->set("force_tff", QString::number(index).toLatin1().constData());
        emit producerChanged(producer());
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
    if (ui->speedSpinBox->value() == Util::GetSpeedFromProducer(producer()))
        return;
    if (ui->speedSpinBox->value() == 1.0) {
        ui->pitchCheckBox->setEnabled(false);
    } else {
        ui->pitchCheckBox->setEnabled(true);
    }
    m_recalcDuration = true;
    recreateProducer();
}

void AvformatProducerWidget::on_pitchCheckBox_stateChanged(int state)
{
    if (!m_producer)
        return;
    if (state == Qt::Unchecked) {
        m_producer->set("warp_pitch", 0);
    } else {
        m_producer->set("warp_pitch", 1);
    }
    emit modified();
}

void AvformatProducerWidget::on_syncSlider_valueChanged(int value)
{
    double delay = double(value) / 1000.0;
    if (m_producer && m_producer->get_double("video_delay") != delay) {
        m_producer->set("video_delay", delay);
        emit modified();
    }
}

void AvformatProducerWidget::on_actionOpenFolder_triggered()
{
    Util::showInFolder(Util::GetFilenameFromProducer(producer()));
}

void AvformatProducerWidget::on_menuButton_clicked()
{
    QMenu menu;
    menu.addAction(ui->actionReset);
    if (!MLT.resource().contains("://")) // not a network stream
        menu.addAction(ui->actionOpenFolder);
    menu.addAction(ui->actionCopyFullFilePath);
    menu.addAction(ui->actionFFmpegInfo);
    menu.addAction(ui->actionFFmpegIntegrityCheck);
    menu.addAction(ui->actionFFmpegConvert);
    menu.addAction(ui->actionExtractSubclip);
    menu.addAction(ui->actionSetFileDate);
    if (Util::GetFilenameFromProducer(producer()).toLower().endsWith(".mp4")) {
        menu.addAction(ui->actionSetEquirectangular);
    }
    menu.addAction(ui->actionFFmpegVideoQuality);
    if (ui->actionExportGPX->isEnabled()) {
        menu.addAction(ui->actionExportGPX);
    }
    menu.exec(ui->menuButton->mapToGlobal(QPoint(0, 0)));
}

void AvformatProducerWidget::on_actionCopyFullFilePath_triggered()
{
    qApp->clipboard()->setText(Util::GetFilenameFromProducer(producer()));
}

void AvformatProducerWidget::on_notesTextEdit_textChanged()
{
    QString existing = QString::fromUtf8(m_producer->get(kCommentProperty));
    if (ui->notesTextEdit->toPlainText() != existing) {
        m_producer->set(kCommentProperty, ui->notesTextEdit->toPlainText().toUtf8().constData());
        emit modified();
    }
}

void AvformatProducerWidget::on_actionFFmpegInfo_triggered()
{
    QStringList args;
    args << "-v" << "quiet";
    args << "-print_format" << "ini";
    args << "-pretty";
    args << "-show_format" << "-show_programs" << "-show_streams" << "-find_stream_info";
    args << Util::GetFilenameFromProducer(producer());
    AbstractJob *job = new FfprobeJob(args.last(), args);
    job->start();
}

void AvformatProducerWidget::on_actionFFmpegIntegrityCheck_triggered()
{
    QString resource = Util::GetFilenameFromProducer(producer());
    QStringList args;
    args << "-xerror";
    args << "-err_detect" << "+explode";
    args << "-v" << "info";
    args << "-i" << resource;
    args << "-map" << "0";
    args << "-f" << "null" << "pipe:";
    JOBS.add(new FfmpegJob(resource, args));
}

void AvformatProducerWidget::on_actionFFmpegConvert_triggered()
{
    TranscodeDialog dialog(
        tr("Choose an edit-friendly format below and then click OK to choose a file name. "
           "After choosing a file name, a job is created. "
           "When it is done, double-click the job to open it.\n"),
        ui->scanComboBox->currentIndex(), this);
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.set709Convert(ui->videoTableWidget->item(5, 1)->data(Qt::UserRole).toInt() > 7);
    dialog.showSubClipCheckBox();
    convert(dialog);
}

static QString getNextFile(const QString &filePath)
{
    QFileInfo info(filePath);
    QString basename = info.completeBaseName();
    QString extension = info.suffix();
    if (extension.isEmpty()) {
        extension = basename;
        basename = QString();
    }
    for (unsigned i = 1; i < std::numeric_limits<unsigned>::max(); i++) {
        QString filename = QString::fromLatin1("%1%2.%3").arg(basename).arg(i).arg(extension);
        if (!info.dir().exists(filename))
            return info.dir().filePath(filename);
    }
    return filePath;
}

void AvformatProducerWidget::convert(TranscodeDialog &dialog)
{
    int result = dialog.exec();
    if (dialog.isCheckBoxChecked()) {
        Settings.setShowConvertClipDialog(false);
    }
    if (result == QDialog::Accepted) {
        QString resource = Util::GetFilenameFromProducer(producer());
        QString path = Settings.savePath();
        QStringList args;
        QString nameFilter;
        int in = -1;

        args << "-loglevel" << "verbose";
        args << "-i" << resource;
        args << "-max_muxing_queue_size" << "9999";

        if (dialog.isSubClip()) {
            if (Settings.proxyEnabled()) {
                m_producer->Mlt::Properties::clear(kOriginalResourceProperty);
            }

            // set trim options
            if (m_producer->get(kFilterInProperty)) {
                in = m_producer->get_int(kFilterInProperty);
                int ss = qMax(0, in - qRound(m_producer->get_fps() * kHandleSeconds));
                auto s = QString::fromLatin1(m_producer->frames_to_time(ss, mlt_time_clock));
                args << "-ss" << s.replace(',', '.');
                in -= ss;
            } else {
                args << "-ss" << QString::fromLatin1(m_producer->get_time("in", mlt_time_clock)).replace(',',
                                                                                                         '.').replace(',', '.');
            }
            if (m_producer->get(kFilterOutProperty)) {
                int out = m_producer->get_int(kFilterOutProperty);
                int to = qMin(m_producer->get_playtime() - 1, out + qRound(m_producer->get_fps() * kHandleSeconds));
                auto s = QString::fromLatin1(m_producer->frames_to_time(to, mlt_time_clock));
                args << "-to" << s.replace(',', '.');
            } else {
                args << "-to" << QString::fromLatin1(m_producer->get_time("out", mlt_time_clock)).replace(',', '.');
            }
        }

        // transcode all streams except data, subtitles, and attachments
        auto audioIndex = m_producer->property_exists(kDefaultAudioIndexProperty) ? m_producer->get_int(
                              kDefaultAudioIndexProperty) : m_producer->get_int("audio_index");
        if (m_producer->get_int("video_index") < audioIndex) {
            args << "-map" << "0:V?" << "-map" << "0:a?";
        } else {
            args << "-map" << "0:a?" << "-map" << "0:V?";
        }
        args << "-map_metadata" << "0" << "-ignore_unknown";

        // Set Sample rate if different than source
        if ( !dialog.sampleRate().isEmpty() ) {
            args << "-ar" << dialog.sampleRate();
        }

        // Set video filters
        args << "-vf";
        QString filterString;
        if (dialog.deinterlace()) {
            QString deinterlaceFilter = QString("bwdif,");
            filterString = filterString + deinterlaceFilter;
        }
        QString range;
        if (ui->rangeComboBox->currentIndex())
            range = "full";
        else
            range = "mpeg";
        if (dialog.get709Convert()) {
            QString convertFilter =
                QString("zscale=t=linear:npl=100,format=gbrpf32le,zscale=p=bt709,tonemap=tonemap=hable:desat=0,zscale=t=bt709:m=bt709:r=tv,format=yuv422p,");
            filterString = filterString + convertFilter;
        }
        filterString = filterString +
                       QString("scale=flags=accurate_rnd+full_chroma_inp+full_chroma_int:in_range=%1:out_range=%2").arg(
                           range).arg(range);
        if (dialog.fpsOverride()) {
            auto fps = QString("%1").arg(dialog.fps(), 0, 'f', 6);
            int numerator, denominator;
            Util::normalizeFrameRate(dialog.fps(), numerator, denominator);
            if (denominator == 1001) {
                fps = QString("%1/%2").arg(numerator).arg(denominator);
            }
            QString minterpFilter =
                QString(",minterpolate='mi_mode=%1:mc_mode=aobmc:me_mode=bidir:vsbmc=1:fps=%2'").arg(
                    dialog.frc()).arg(fps);
            filterString = filterString + minterpFilter;
        }
        args << filterString;

        // Specify color range
        if (ui->rangeComboBox->currentIndex())
            args << "-color_range" << "jpeg";
        else
            args << "-color_range" << "mpeg";

        if (!dialog.deinterlace() && !ui->scanComboBox->currentIndex())
            args << "-flags" << "+ildct+ilme" << "-top" << QString::number(
                     ui->fieldOrderComboBox->currentIndex());

        switch (dialog.format()) {
        case 0:
            path.append("/%1 - %2.mp4");
            nameFilter = tr("MP4 (*.mp4);;All Files (*)");
            args << "-f" << "mp4" << "-codec:a" << "ac3" << "-b:a" << "512k" << "-codec:v" << "libx264";
            args << "-preset" << "medium" << "-g" << "1" << "-crf" << "15";
            break;
        case 1:
            args << "-f" << "mov" << "-codec:a" << "pcm_f32le";
            if (dialog.deinterlace() || ui->scanComboBox->currentIndex()) { // progressive
                args << "-codec:v" << "dnxhd" << "-profile:v" << "dnxhr_hq" << "-pix_fmt" << "yuv422p";
            } else { // interlaced
                args << "-codec:v" << "prores_ks" << "-profile:v" << "standard";
            }
            path.append("/%1 - %2.mov");
            nameFilter = tr("MOV (*.mov);;All Files (*)");
            break;
        case 2:
            args << "-f" << "matroska" << "-codec:a" << "pcm_f32le" << "-codec:v" << "utvideo";
            args << "-pix_fmt" << "yuv422p";
            path.append("/%1 - %2.mkv");
            nameFilter = tr("MKV (*.mkv);;All Files (*)");
            break;
        }
        if (dialog.get709Convert()) {
            args << "-colorspace" << "bt709" << "-color_primaries" << "bt709" << "-color_trc" << "bt709";
        }
        QFileInfo fi(resource);
        QString suffix = dialog.isSubClip() ? tr("Sub-clip") + ' ' : tr("Converted");
        path = path.arg(fi.completeBaseName(), suffix);
        if (dialog.isSubClip()) {
            path = getNextFile(path);
        }
        QString filename = QFileDialog::getSaveFileName(this, dialog.windowTitle(), path, nameFilter,
                                                        nullptr, Util::getFileDialogOptions());
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

            if (Util::warnIfLowDiskSpace(filename)) {
                MAIN.showStatusMessage(tr("Convert canceled"));
                return;
            }

            Settings.setSavePath(QFileInfo(filename).path());
            args << "-y" << filename;
            m_producer->Mlt::Properties::clear(kOriginalResourceProperty);

            FfmpegJob *job = new FfmpegJob(filename, args, false);
            job->setLabel(tr("Convert %1").arg(Util::baseName(filename)));
            if (dialog.isSubClip()) {
                if (m_producer->get(kMultitrackItemProperty)) {
                    QString s = QString::fromLatin1(m_producer->get(kMultitrackItemProperty));
                    auto parts = s.split(':');
                    if (parts.length() == 2) {
                        int clipIndex = parts[0].toInt();
                        int trackIndex = parts[1].toInt();
                        QUuid uuid = MAIN.timelineClipUuid(trackIndex, clipIndex);
                        if (!uuid.isNull()) {
                            job->setPostJobAction(new ReplaceOnePostJobAction(resource, filename, QString(), uuid,
                                                                              in));
                            JOBS.add(job);
                        }
                    }
                } else {
                    job->setPostJobAction(new OpenPostJobAction(resource, filename, QString()));
                    JOBS.add(job);
                }
                return;
            }
            job->setPostJobAction(new ReplaceAllPostJobAction(resource, filename, Util::getHash(*m_producer)));
            JOBS.add(job);
        }
    }
}

bool AvformatProducerWidget::revertToOriginalResource()
{
    QString resource = m_producer->get(kOriginalResourceProperty);
    if (!resource.isEmpty() && !m_producer->get_int(kIsProxyProperty)) {
        m_producer->Mlt::Properties::clear(kOriginalResourceProperty);
        if (m_producer->get(kMultitrackItemProperty)) {
            QString s = QString::fromLatin1(m_producer->get(kMultitrackItemProperty));
            auto parts = s.split(':');
            if (parts.length() == 2) {
                int clipIndex = parts[0].toInt();
                int trackIndex = parts[1].toInt();
                QUuid uuid = MAIN.timelineClipUuid(trackIndex, clipIndex);
                if (!uuid.isNull()) {
                    Mlt::Producer newProducer(MLT.profile(), resource.toUtf8().constData());
                    if (newProducer.is_valid()) {
                        Mlt::Producer *producer = MLT.setupNewProducer(&newProducer);
                        producer->set(kIsProxyProperty, 1);
                        producer->set(kOriginalResourceProperty, resource.toUtf8().constData());
                        producer->set_in_and_out(m_producer->get_int(kOriginalInProperty),
                                                 m_producer->get_int(kOriginalOutProperty));
                        MAIN.replaceInTimeline(uuid, *producer);
                        delete producer;
                        return true;
                    }
                }
            }
        } else {
            MAIN.open(resource);
            return true;
        }
    }
    return false;
}

void AvformatProducerWidget::setSyncVisibility()
{
    bool visible = ui->tabWidget->isTabEnabled(0) && ui->tabWidget->isTabEnabled(1) &&
                   m_producer->get_int("video_index") != -1;
    ui->syncSlider->setVisible(visible);
    ui->syncLabel->setVisible(visible);
    ui->syncSpinBox->setVisible(visible);
}

void AvformatProducerWidget::on_reverseButton_clicked()
{
    if (revertToOriginalResource())
        return;

    TranscodeDialog dialog(
        tr("Choose an edit-friendly format below and then click OK to choose a file name. "
           "After choosing a file name, a job is created. "
           "When it is done, double-click the job to open it.\n"),
        ui->scanComboBox->currentIndex(), this);
    dialog.setWindowTitle(tr("Reverse..."));
    dialog.setWindowModality(QmlApplication::dialogModality());
    int result = dialog.exec();
    if (dialog.isCheckBoxChecked()) {
        Settings.setShowConvertClipDialog(false);
    }
    if (result == QDialog::Accepted) {
        QString resource = Util::GetFilenameFromProducer(producer());
        QString path = Settings.savePath();
        QStringList meltArgs;
        QStringList ffmpegArgs;
        QString nameFilter;
        QString ffmpegSuffix = "mov";
        int in = -1;

        if (Settings.proxyEnabled()) {
            m_producer->Mlt::Properties::clear(kOriginalResourceProperty);
        } else {
            // Save these properties for revertToOriginalResource()
            m_producer->set(kOriginalResourceProperty, resource.toUtf8().constData());
            m_producer->set(kOriginalInProperty, m_producer->get(kFilterInProperty) ?
                            m_producer->get_time(kFilterInProperty, mlt_time_clock) : m_producer->get_time("in",
                                                                                                           mlt_time_clock));
            m_producer->set(kOriginalOutProperty, m_producer->get(kFilterOutProperty) ?
                            m_producer->get_time(kFilterOutProperty, mlt_time_clock) : m_producer->get_time("out",
                                                                                                            mlt_time_clock));
        }

        ffmpegArgs << "-loglevel" << "verbose";
        ffmpegArgs << "-i" << resource;
        ffmpegArgs << "-max_muxing_queue_size" << "9999";

        // set trim options
        if (m_producer->get(kFilterInProperty)) {
            in = m_producer->get_int(kFilterInProperty);
            int ss = qMax(0, in - qRound(m_producer->get_fps() * kHandleSeconds));
            auto s = QString::fromLatin1(m_producer->frames_to_time(ss, mlt_time_clock));
            ffmpegArgs << "-ss" << s.replace(',', '.');
        } else {
            ffmpegArgs << "-ss" << QString::fromLatin1(m_producer->get_time("in", mlt_time_clock)).replace(',',
                                                                                                           '.').replace(',', '.');
        }
        if (m_producer->get(kFilterOutProperty)) {
            int out = m_producer->get_int(kFilterOutProperty);
            int to = qMin(m_producer->get_playtime() - 1, out + qRound(m_producer->get_fps() * kHandleSeconds));
            in = to - out - 1;
            auto s = QString::fromLatin1(m_producer->frames_to_time(to, mlt_time_clock));
            ffmpegArgs << "-to" << s.replace(',', '.');
        } else {
            ffmpegArgs << "-to" << QString::fromLatin1(m_producer->get_time("out", mlt_time_clock)).replace(',',
                                                                                                            '.');
        }

        // transcode all streams except data, subtitles, and attachments
        ffmpegArgs << "-map" << "0:V?" << "-map" << "0:a?" << "-map_metadata" << "0" << "-ignore_unknown";
        if (ui->rangeComboBox->currentIndex())
            ffmpegArgs << "-vf" <<
                       "scale=flags=accurate_rnd+full_chroma_inp+full_chroma_int:in_range=full:out_range=full" <<
                       "-color_range" << "jpeg";
        else
            ffmpegArgs << "-vf" <<
                       "scale=flags=accurate_rnd+full_chroma_inp+full_chroma_int:in_range=mpeg:out_range=mpeg" <<
                       "-color_range" << "mpeg";
        if (!ui->scanComboBox->currentIndex())
            ffmpegArgs << "-flags" << "+ildct+ilme" << "-top" << QString::number(
                           ui->fieldOrderComboBox->currentIndex());

        meltArgs << "-consumer" << "avformat";
        if (m_producer->get_int("audio_index") == -1) {
            meltArgs << "an=1" << "audio_off=1";
        } else if (qstrcmp(m_producer->get("audio_index"), "all")) {
            int index = m_producer->get_int("audio_index");
            QString key = QString("meta.media.%1.codec.channels").arg(index);
            const char *channels = m_producer->get(key.toLatin1().constData());
            meltArgs << QString("channels=").append(channels);
        }
        if (m_producer->get_int("video_index") == -1)
            meltArgs << "vn=1" << "video_off=1";

        ffmpegArgs << "-f" << "mov" << "-codec:a" << "pcm_f32le";

        switch (dialog.format()) {
        case 0:
            path.append("/%1 - %2.mp4");
            nameFilter = tr("MP4 (*.mp4);;All Files (*)");
            if (ui->scanComboBox->currentIndex()) { // progressive
                ffmpegArgs << "-codec:v" << "dnxhd" << "-profile:v" << "dnxhr_hq" << "-pix_fmt" << "yuv422p";
            } else { // interlaced
                ffmpegArgs << "-codec:v" << "prores_ks" << "-profile:v" << "standard";
                meltArgs << "top_field_first=" + QString::number(ui->fieldOrderComboBox->currentIndex());
            }
            meltArgs << "acodec=ac3" << "ab=512k" << "vcodec=libx264";
            meltArgs << "vpreset=medium" << "g=1" << "crf=11";
            break;
        case 1:
            meltArgs << "acodec=alac";
            if (ui->scanComboBox->currentIndex()) { // progressive
                ffmpegArgs << "-codec:v" << "dnxhd" << "-profile:v" << "dnxhr_hq" << "-pix_fmt" << "yuv422p";
                meltArgs << "vcodec=dnxhd" << "vprofile=dnxhr_hq";
            } else { // interlaced
                ffmpegArgs << "-codec:v" << "prores_ks" << "-profile:v" << "standard";
                meltArgs << "top_field_first=" + QString::number(ui->fieldOrderComboBox->currentIndex());
                meltArgs << "vcodec=prores_ks" << "vprofile=standard";
            }
            path.append("/%1 - %2.mov");
            nameFilter = tr("MOV (*.mov);;All Files (*)");
            break;
        case 2:
            ffmpegArgs << "-codec:v" << "utvideo" << "-pix_fmt" << "yuv422p";
            if (!ui->scanComboBox->currentIndex()) { // interlaced
                meltArgs << "field_order=" + QString::fromLatin1(ui->fieldOrderComboBox->currentIndex() ? "tt" :
                                                                 "bb");
            }
            meltArgs << "acodec=pcm_f32le" << "vcodec=utvideo" << "mlt_audio_format=f32le" << "pix_fmt=yuv422p";
            path.append("/%1 - %2.mkv");
            nameFilter = tr("MKV (*.mkv);;All Files (*)");
            break;
        }
        QFileInfo fi(resource);
        path = path.arg(fi.completeBaseName()).arg(tr("Reversed"));
        QString filename = QmlApplication::getNextProjectFile(path);
        if (filename.isEmpty()) {
            filename = QFileDialog::getSaveFileName(this, dialog.windowTitle(), path, nameFilter,
                                                    nullptr, Util::getFileDialogOptions());
        }
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

            if (Util::warnIfLowDiskSpace(filename)) {
                MAIN.showStatusMessage(tr("Reverse canceled"));
                return;
            }

            Settings.setSavePath(QFileInfo(filename).path());

            // Make a temporary file name for the ffmpeg job.
            QFileInfo fi(filename);
            QString tmpFileName = QString("%1/%2 - XXXXXX.%3").arg(fi.path()).arg(fi.completeBaseName()).arg(
                                      ffmpegSuffix);
            QTemporaryFile tmp(tmpFileName);
            tmp.setAutoRemove(false);
            tmp.open();
            tmp.close();
            tmpFileName = tmp.fileName();

            // Run the ffmpeg job to convert a portion of the file to something edit-friendly.
            ffmpegArgs << "-y" << tmpFileName;
            FfmpegJob *ffmpegJob = new FfmpegJob(filename, ffmpegArgs, false);
            ffmpegJob->setLabel(tr("Convert %1").arg(Util::baseName(resource)));
            JOBS.add(ffmpegJob);

            // Run the melt job to convert the intermediate file to the reversed clip.
            meltArgs.prepend(QString("timewarp:-1.0:").append(tmpFileName));
            meltArgs << QString("target=").append(filename);
            MeltJob *meltJob = new MeltJob(filename, meltArgs,
                                           m_producer->get_int("meta.media.frame_rate_num"), m_producer->get_int("meta.media.frame_rate_den"));
            meltJob->setLabel(tr("Reverse %1").arg(Util::baseName(resource)));

            if (m_producer->get(kMultitrackItemProperty)) {
                QString s = QString::fromLatin1(m_producer->get(kMultitrackItemProperty));
                auto parts = s.split(':');
                if (parts.length() == 2) {
                    int clipIndex = parts[0].toInt();
                    int trackIndex = parts[1].toInt();
                    QUuid uuid = MAIN.timelineClipUuid(trackIndex, clipIndex);
                    if (!uuid.isNull()) {
                        meltJob->setPostJobAction(new ReplaceOnePostJobAction(resource, filename, tmpFileName,
                                                                              uuid, in));
                        JOBS.add(meltJob);
                        return;
                    }
                }
            }
            meltJob->setPostJobAction(new OpenPostJobAction(resource, filename, tmpFileName));
            JOBS.add(meltJob);
        }
    }
}


void AvformatProducerWidget::on_actionExtractSubclip_triggered()
{
    QString resource = Util::GetFilenameFromProducer(producer());
    QString path = Settings.savePath();
    QFileInfo fi(resource);

    path.append("/%1 - %2.%3");
    path = path.arg(fi.completeBaseName()).arg(tr("Sub-clip")).arg(fi.suffix());
    QString caption = tr("Extract Sub-clip...");
    QString nameFilter = tr("%1 (*.%2);;All Files (*)").arg(fi.suffix()).arg(fi.suffix());
    QString filename = QFileDialog::getSaveFileName(this, caption, path, nameFilter,
                                                    nullptr, Util::getFileDialogOptions());

    if (!filename.isEmpty()) {
        if (filename == QDir::toNativeSeparators(resource)) {
            QMessageBox::warning(this, caption,
                                 QObject::tr("Unable to write file %1\n"
                                             "Perhaps you do not have permission.\n"
                                             "Try again with a different folder.")
                                 .arg(fi.fileName()));
            return;
        }
        if (Util::warnIfNotWritable(filename, this, caption))
            return;
        Settings.setSavePath(QFileInfo(filename).path());

        QStringList ffmpegArgs;

        // Build the ffmpeg command line.
        ffmpegArgs << "-loglevel" << "verbose";
        ffmpegArgs << "-i" << resource;
        // set trim options
        if (m_producer->get_int(kFilterInProperty) || m_producer->get_int("in")) {
            if (m_producer->get(kFilterInProperty))
                ffmpegArgs << "-ss" << QString::fromLatin1(m_producer->get_time(kFilterInProperty,
                                                                                mlt_time_clock)).replace(',', '.');
            else
                ffmpegArgs << "-ss" << QString::fromLatin1(m_producer->get_time("in", mlt_time_clock)).replace(',',
                                                                                                               '.').replace(',', '.');
        }
        if (m_producer->get(kFilterOutProperty))
            ffmpegArgs << "-to" << QString::fromLatin1(m_producer->get_time(kFilterOutProperty,
                                                                            mlt_time_clock)).replace(',', '.');
        else
            ffmpegArgs << "-to" << QString::fromLatin1(m_producer->get_time("out", mlt_time_clock)).replace(',',
                                                                                                            '.');
        ffmpegArgs << "-avoid_negative_ts" << "make_zero"
                   << "-map" << "0:V?" << "-map" << "0:a?" << "-map" << "0:s?"
                   << "-map_metadata" << "0"
                   << "-codec" << "copy" << "-y" << filename;

        // Run the ffmpeg job.
        FfmpegJob *ffmpegJob = new FfmpegJob(filename, ffmpegArgs, false);
        ffmpegJob->setLabel(tr("Extract sub-clip %1").arg(Util::baseName(resource)));
        JOBS.add(ffmpegJob);
    }
}


void AvformatProducerWidget::on_actionSetFileDate_triggered()
{
    QString resource = Util::GetFilenameFromProducer(producer());
    FileDateDialog dialog(resource, producer(), this);
    dialog.setModal(QmlApplication::dialogModality());
    dialog.exec();
}

void AvformatProducerWidget::on_rangeComboBox_activated(int index)
{
    if (m_producer) {
        m_producer->set("color_range", index ? 2 : 1);
        recreateProducer();
    }
}

void AvformatProducerWidget::on_filenameLabel_editingFinished()
{
    if (m_producer) {
        const auto caption = ui->filenameLabel->text();
        if (caption.isEmpty()) {
            double warpSpeed = Util::GetSpeedFromProducer(producer());
            QString resource = Util::GetFilenameFromProducer(producer());
            QString caption = Util::baseName(resource, true);
            if (warpSpeed != 1.0)
                caption = QString("%1 (%2x)").arg(caption).arg(warpSpeed);
            m_producer->set(kShotcutCaptionProperty, caption.toUtf8().constData());
            ui->filenameLabel->setText(caption);
        } else {
            m_producer->set(kShotcutCaptionProperty, caption.toUtf8().constData());
        }
        emit modified();
    }
}

void AvformatProducerWidget::on_convertButton_clicked()
{
    on_actionFFmpegConvert_triggered();
}

void AvformatProducerWidget::on_actionDisableProxy_triggered(bool checked)
{
    if (checked) {
        producer()->set(kDisableProxyProperty, 1);

        // Replace with original
        if (producer()->get_int(kIsProxyProperty) && producer()->get(kOriginalResourceProperty)) {
            Mlt::Producer original(MLT.profile(), producer()->get(kOriginalResourceProperty));
            if (original.is_valid()) {
                Mlt::Producer *producer = MLT.setupNewProducer(&original);
                producer->set(kDisableProxyProperty, 1);
                MAIN.replaceAllByHash(Util::getHash(original), *producer, true);
                delete producer;
            }
        }
    } else {
        producer()->Mlt::Properties::clear(kDisableProxyProperty);
        ui->actionMakeProxy->setEnabled(true);
    }
}

void AvformatProducerWidget::on_actionMakeProxy_triggered()
{
    bool fullRange = ui->rangeComboBox->currentIndex() == 1;
    QPoint aspectRatio(ui->aspectNumSpinBox->value(), ui->aspectDenSpinBox->value());
    ProxyManager::ScanMode scan = ProxyManager::Progressive;
    if (!ui->scanComboBox->currentIndex())
        scan = ui->fieldOrderComboBox->currentIndex() ? ProxyManager::InterlacedTopFieldFirst
               : ProxyManager::InterlacedBottomFieldFirst;

    ProxyManager::generateVideoProxy(*producer(), fullRange, scan, aspectRatio);
}

void AvformatProducerWidget::on_actionDeleteProxy_triggered()
{
    // Delete the file if it exists
    QString hash = Util::getHash(*producer());
    QString fileName = hash + ProxyManager::videoFilenameExtension();
    QDir dir = ProxyManager::dir();
    LOG_DEBUG() << "removing" << dir.filePath(fileName);
    dir.remove(dir.filePath(fileName));

    // Delete the pending file if it exists));
    fileName = hash + ProxyManager::pendingVideoExtension();
    dir.remove(dir.filePath(fileName));

    // Replace with original
    if (producer()->get_int(kIsProxyProperty) && producer()->get(kOriginalResourceProperty)) {
        Mlt::Producer original(MLT.profile(), producer()->get(kOriginalResourceProperty));
        if (original.is_valid()) {
            Mlt::Producer *producer = MLT.setupNewProducer(&original);
            MAIN.replaceAllByHash(hash, *producer, true);
            delete producer;
        }
    }
}

void AvformatProducerWidget::on_actionCopyHashCode_triggered()
{
    qApp->clipboard()->setText(Util::getHash(*producer()));
    QMessageBox::information(this, qApp->applicationName(),
                             tr("The hash code below is already copied to your clipboard:\n\n") +
                             Util::getHash(*producer()),
                             QMessageBox::Ok);
}

void AvformatProducerWidget::on_proxyButton_clicked()
{
    if (m_producer->get_int("video_index") >= 0) {
        QMenu menu;
        if (ProxyManager::isValidVideo(*producer())) {
            menu.addAction(ui->actionMakeProxy);
        }
#ifndef Q_OS_WIN
        menu.addAction(ui->actionDeleteProxy);
#endif
        menu.addAction(ui->actionDisableProxy);
        menu.addAction(ui->actionCopyHashCode);
        if (m_producer->get_int(kDisableProxyProperty)) {
            ui->actionMakeProxy->setDisabled(true);
            ui->actionDisableProxy->setChecked(true);
        }
        menu.exec(ui->proxyButton->mapToGlobal(QPoint(0, 0)));
    }
}

void AvformatProducerWidget::on_actionReset_triggered()
{
    ui->speedSpinBox->setValue(1.0);
    ui->pitchCheckBox->setCheckState(Qt::Unchecked);
    Mlt::Producer *p = newProducer(MLT.profile());
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

void AvformatProducerWidget::on_actionSetEquirectangular_triggered()
{
    // Get the location and file name for the report.
    QString caption = tr("Set Equirectangular Projection");
    QFileInfo info(Util::GetFilenameFromProducer(producer()));
    QString directory = QString("%1/%2 - ERP.%3")
                        .arg(info.path())
                        .arg(info.completeBaseName())
                        .arg(info.suffix());
    QString filePath = QFileDialog::getSaveFileName(&MAIN, caption, directory, QString(),
                                                    nullptr, Util::getFileDialogOptions());
    if (!filePath.isEmpty()) {
        if (SpatialMedia::injectSpherical(info.filePath().toStdString(), filePath.toStdString())) {
            MAIN.showStatusMessage(tr("Successfully wrote %1").arg(QFileInfo(filePath).fileName()));
        } else {
            MAIN.showStatusMessage(tr("An error occurred saving the projection."));
        }
    }
}

void AvformatProducerWidget::on_actionFFmpegVideoQuality_triggered()
{
    QString caption = tr("Choose the Other Video");
    QFileInfo info(Util::GetFilenameFromProducer(producer()));
    QString directory = QString("%1/%2 - ERP.%3").arg(info.path(), info.completeBaseName(),
                                                      info.suffix());
    QString filePath = QFileDialog::getOpenFileName(&MAIN, caption, directory, QString(),
                                                    nullptr, Util::getFileDialogOptions());
    if (!filePath.isEmpty()) {
        QString resource = Util::GetFilenameFromProducer(producer());
        QDir dir = QmlApplication::dataDir();
        dir.cd("vmaf");
        QStringList args;
        args << "-hide_banner";
        args << "-i" << resource;
        args << "-i" << filePath;
        args << "-filter_complex";
        int width = m_producer->get_int("meta.media.width");
        int height = m_producer->get_int("meta.media.height");
        double fps = m_producer->get_double("meta.media.frame_rate_num");
        int frameRateNum, frameRateDen;
        if (m_producer->get_double("meta.media.frame_rate_den") > 0) {
            fps /= m_producer->get_double("meta.media.frame_rate_den");
        }
        if (m_producer->get("force_fps")) {
            fps = m_producer->get_double("fps");
        }
        Util::normalizeFrameRate(fps, frameRateNum, frameRateDen);
        auto colorRange = (ui->rangeComboBox->currentIndex() == 1) ? "full" : "limited";

#ifdef Q_OS_WIN
        auto logPath = "con\\:";
        auto modelPath = (width < 3840
                          && height < 2160) ? "share/vmaf/vmaf_v0.6.1.json" : "share/vmaf/vmaf_4k_v0.6.1.json";
#else
        auto logPath = "/dev/stderr";
        auto modelPath = (width < 3840
                          && height < 2160) ? dir.filePath("vmaf_v0.6.1.json") : dir.filePath("vmaf_4k_v0.6.1.json");
#endif
        args << QString("[0:v]fps=%4/%5,setpts=PTS-STARTPTS[reference];[1:v]scale=%1:%2:out_range=%6:flags=bicubic,fps=%4/%5,setpts=PTS-STARTPTS[distorted];[distorted][reference]libvmaf=log_fmt=csv:log_path='%8':feature='name=psnr|name=float_ssim':shortest=true:n_threads=%7:model='path=%3'")
             .arg(width).arg(height)
             .arg(modelPath)
             .arg(frameRateNum).arg(frameRateDen)
             .arg(colorRange)
             .arg(qRound(QThread::idealThreadCount() / 2.))
             .arg(logPath);
        args << "-f" << "null" << "pipe:";
        FfmpegJob *job = new FfmpegJob(resource, args);
        job->setWorkingDirectory(qApp->applicationDirPath());
        job->setLabel(tr("Measure %1").arg(Util::baseName(filePath)));
        JOBS.add(job);
    }
}

void AvformatProducerWidget::on_rotationComboBox_activated(int index)
{
    if (m_producer) {
        MLT.stop();
        m_producer->set("rotate", index * 90);
        recreateProducer();
    }
}

void AvformatProducerWidget::on_actionExportGPX_triggered()
{
    QString resource = Util::GetFilenameFromProducer(producer());
    QStringList args;
    args << "-s";
    args << resource;
    JOBS.add(new GoPro2GpxJob(resource, args));
}

void AvformatProducerWidget::on_speedComboBox_textActivated(const QString &arg1)
{
    if (arg1.isEmpty()) return;
    ui->speedSpinBox->setValue(arg1.toDouble());
    on_speedSpinBox_editingFinished();
}
