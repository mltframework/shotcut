/*
 * Copyright (c) 2012-2016 Meltytech, LLC
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

#include "encodedock.h"
#include "ui_encodedock.h"
#include "dialogs/addencodepresetdialog.h"
#include "jobqueue.h"
#include "mltcontroller.h"
#include "mainwindow.h"
#include "settings.h"
#include "qmltypes/qmlapplication.h"
#include "jobs/encodejob.h"
#include "shotcut_mlt_properties.h"

#include <QtDebug>
#include <QtWidgets>
#include <QtXml>
#include <QtMath>
#include <QTimer>

// formulas to map absolute value ranges to percentages as int
#define TO_ABSOLUTE(min, max, rel) qRound(float(min) + float((max) - (min) + 1) * float(rel) / 100.0f)
#define TO_RELATIVE(min, max, abs) qRound(100.0f * float((abs) - (min)) / float((max) - (min) + 1))

static double getBufferSize(Mlt::Properties& preset, const char* property);

EncodeDock::EncodeDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::EncodeDock),
    m_presets(Mlt::Repository::presets()),
    m_immediateJob(0),
    m_profiles(Mlt::Profile::list())
{
    qDebug() << "begin";
    ui->setupUi(this);
    ui->stopCaptureButton->hide();
    ui->videoCodecThreadsSpinner->setMaximum(QThread::idealThreadCount());
    if (QThread::idealThreadCount() < 3)
        ui->parallelCheckbox->setHidden(true);
    toggleViewAction()->setIcon(windowIcon());

    connect(ui->videoBitrateCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(on_videoBufferDurationChanged()));
    connect(ui->videoBufferSizeSpinner, SIGNAL(valueChanged(double)), this, SLOT(on_videoBufferDurationChanged()));

    m_presetsModel.setSourceModel(new QStandardItemModel(this));
    m_presetsModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->presetsTree->setModel(&m_presetsModel);
    loadPresets();

    // populate the combos
    Mlt::Consumer c(MLT.profile(), "avformat");
    c.set("f", "list");
    c.set("acodec", "list");
    c.set("vcodec", "list");
    c.start();
    c.stop();

    Mlt::Properties* p = new Mlt::Properties(c.get_data("f"));
    ui->formatCombo->blockSignals(true);
    for (int i = 0; i < p->count(); i++)
        ui->formatCombo->addItem(p->get(i));
    delete p;
    ui->formatCombo->model()->sort(0);
    ui->formatCombo->insertItem(0, tr("Automatic from extension"));
    ui->formatCombo->blockSignals(false);

    p = new Mlt::Properties(c.get_data("acodec"));
    for (int i = 0; i < p->count(); i++)
        ui->audioCodecCombo->addItem(p->get(i));
    delete p;
    ui->audioCodecCombo->model()->sort(0);
    ui->audioCodecCombo->insertItem(0, tr("Default for format"));

    p = new Mlt::Properties(c.get_data("vcodec"));
    for (int i = 0; i < p->count(); i++)
        ui->videoCodecCombo->addItem(p->get(i));
    delete p;
    ui->videoCodecCombo->model()->sort(0);
    ui->videoCodecCombo->insertItem(0, tr("Default for format"));

    on_resetButton_clicked();

    qDebug() << "end";
}

EncodeDock::~EncodeDock()
{
    delete ui;
    delete m_presets;
    delete m_profiles;
}

void EncodeDock::loadPresetFromProperties(Mlt::Properties& preset)
{
    int audioQuality = -1;
    int videoQuality = -1;
    QStringList other;

    ui->disableAudioCheckbox->setChecked(preset.get_int("an"));
    ui->disableVideoCheckbox->setChecked(preset.get_int("vn"));
    m_extension.clear();
    for (int i = 0; i < preset.count(); i++) {
        QString name(preset.get_name(i));
        if (name == "f") {
            for (int i = 0; i < ui->formatCombo->count(); i++)
                if (ui->formatCombo->itemText(i) == preset.get("f")) {
                    ui->formatCombo->blockSignals(true);
                    ui->formatCombo->setCurrentIndex(i);
                    ui->formatCombo->blockSignals(false);
                    break;
                }
        }
        else if (name == "acodec") {
            for (int i = 0; i < ui->audioCodecCombo->count(); i++)
                if (ui->audioCodecCombo->itemText(i) == preset.get("acodec"))
                    ui->audioCodecCombo->setCurrentIndex(i);
            if (ui->audioCodecCombo->currentText() == "libopus")
                // reset libopus to VBR (its default)
                ui->audioRateControlCombo->setCurrentIndex(RateControlQuality);
        }
        else if (name == "vcodec") {
            for (int i = 0; i < ui->videoCodecCombo->count(); i++)
                if (ui->videoCodecCombo->itemText(i) == preset.get("vcodec"))
                    ui->videoCodecCombo->setCurrentIndex(i);
        }
        else if (name == "ar")
            ui->sampleRateCombo->lineEdit()->setText(preset.get("ar"));
        else if (name == "ab")
            ui->audioBitrateCombo->lineEdit()->setText(preset.get("ab"));
        else if (name == "vb") {
            ui->videoRateControlCombo->setCurrentIndex(RateControlAverage);
            ui->videoBitrateCombo->lineEdit()->setText(preset.get("vb"));
        }
        else if (name == "g")
            ui->gopSpinner->setValue(preset.get_int("g"));
        else if (name == "bf")
            ui->bFramesSpinner->setValue(preset.get_int("bf"));
        else if (name == "deinterlace") {
            ui->scanModeCombo->setCurrentIndex(preset.get_int("deinterlace"));
            ui->scanModeCombo->setEnabled(false);
        }
        else if (name == "progressive") {
            ui->scanModeCombo->setCurrentIndex(preset.get_int("progressive"));
            ui->scanModeCombo->setEnabled(false);
        }
        else if (name == "top_field_first") {
            ui->fieldOrderCombo->setCurrentIndex(preset.get_int("top_field_first"));
        }
        else if (name == "width") {
            ui->widthSpinner->setValue(preset.get_int("width"));
            ui->widthSpinner->setEnabled(false);
        }
        else if (name == "height") {
            ui->heightSpinner->setValue(preset.get_int("height"));
            ui->heightSpinner->setEnabled(false);
        }
        else if (name == "aspect") {
            double dar = preset.get_double("aspect");
            if (int(dar * 100) == 133) {
                ui->aspectNumSpinner->setValue(4);
                ui->aspectDenSpinner->setValue(3);
            }
            else if (int(dar * 100) == 177) {
                ui->aspectNumSpinner->setValue(16);
                ui->aspectDenSpinner->setValue(9);
            }
            else {
                ui->aspectNumSpinner->setValue(dar * 1000);
                ui->aspectDenSpinner->setValue(1000);
            }
            ui->aspectNumSpinner->setEnabled(false);
            ui->aspectDenSpinner->setEnabled(false);
        }
        else if (name == "r") {
            ui->fpsSpinner->setValue(preset.get_double("r"));
            ui->fpsSpinner->setEnabled(false);
        }
        else if (name == "pix_fmt") {
            QString pix_fmt(preset.get("pix_fmt"));
            if (pix_fmt != "yuv420p")
                other.append(QString("%1=%2").arg(name).arg(pix_fmt));
        }
        else if (name == "pass")
            ui->dualPassCheckbox->setChecked(true);
        else if (name == "aq") {
            ui->audioRateControlCombo->setCurrentIndex(RateControlQuality);
            audioQuality = preset.get_int("aq");
        }
        else if (name == "vbr") {
            // libopus rate mode
            QString value(preset.get("vbr"));
            if (value == "off")
                ui->audioRateControlCombo->setCurrentIndex(RateControlConstant);
            else if (value == "constrained")
                ui->audioRateControlCombo->setCurrentIndex(RateControlAverage);
            else
                ui->audioRateControlCombo->setCurrentIndex(RateControlQuality);
        }
        else if (name == "vq") {
            ui->videoRateControlCombo->setCurrentIndex(RateControlQuality);
            videoQuality = preset.get_int("vq");
        }
        else if (name == "qscale") {
            ui->videoRateControlCombo->setCurrentIndex(RateControlQuality);
            videoQuality = preset.get_int("qscale");
        }
        else if (name == "crf") {
            ui->videoRateControlCombo->setCurrentIndex(RateControlQuality);
            videoQuality = preset.get_int("crf");
        }
        else if (name == "bufsize") {
            // traditionally this means video only
            ui->videoRateControlCombo->setCurrentIndex(RateControlConstant);
            ui->videoBufferSizeSpinner->setValue(getBufferSize(preset, "bufsize"));
        }
        else if (name == "vbufsize") {
            ui->videoRateControlCombo->setCurrentIndex(RateControlConstant);
            ui->videoBufferSizeSpinner->setValue(getBufferSize(preset, "vbufsize"));
        }
        else if (name == "threads") {
            // TODO: should we save the thread count and restore it if preset is not 1?
            if (preset.get_int("threads") == 1)
                ui->videoCodecThreadsSpinner->setValue(1);
        }
        else if (name == "meta.preset.extension") {
            m_extension = preset.get("meta.preset.extension");
        }
        else if (name == "deinterlace_method") {
            name = preset.get("deinterlace_method");
            if (name == "onefield")
                ui->deinterlacerCombo->setCurrentIndex(0);
            else if (name == "linearblend")
                ui->deinterlacerCombo->setCurrentIndex(1);
            else if (name == "yadif-nospatial")
                ui->deinterlacerCombo->setCurrentIndex(2);
            else if (name == "yadif")
                ui->deinterlacerCombo->setCurrentIndex(3);
            ui->deinterlacerCombo->setDisabled(true);
        }
        else if (name == "rescale") {
            name = preset.get("rescale");
            if (name == "nearest" || name == "neighbor")
                ui->interpolationCombo->setCurrentIndex(0);
            else if (name == "bilinear")
                ui->interpolationCombo->setCurrentIndex(1);
            else if (name == "bicubic")
                ui->interpolationCombo->setCurrentIndex(2);
            else if (name == "hyper" || name == "lanczos")
                ui->interpolationCombo->setCurrentIndex(3);
            ui->interpolationCombo->setDisabled(true);
        }
        else if (name != "an" && name != "vn" && name != "threads"
                 && !name.startsWith('_') && !name.startsWith("meta.preset.")) {
            other.append(QString("%1=%2").arg(name).arg(preset.get(i)));
        }
    }
    ui->advancedTextEdit->setPlainText(other.join("\n"));

    // normalize the quality settings
    // quality depends on codec
    if (ui->audioRateControlCombo->currentIndex() == RateControlQuality && audioQuality > -1) {
        const QString& acodec = ui->audioCodecCombo->currentText();
        if (acodec == "libmp3lame") // 0 (best) - 9 (worst)
            ui->audioQualitySpinner->setValue(TO_RELATIVE(9, 0, audioQuality));
        if (acodec == "libvorbis" || acodec == "vorbis") // 0 (worst) - 10 (best)
            ui->audioQualitySpinner->setValue(TO_RELATIVE(0, 10, audioQuality));
        else
            // aac: 0 (worst) - 500 (best)
            ui->audioQualitySpinner->setValue(TO_RELATIVE(0, 500, audioQuality));
    }
    if (ui->videoRateControlCombo->currentIndex() == RateControlQuality && videoQuality > -1) {
        const QString& vcodec = ui->videoCodecCombo->currentText();
        //val = min + (max - min) * paramval;
        if (vcodec == "libx264" || vcodec == "libx265") // 0 (best, 100%) - 51 (worst)
            ui->videoQualitySpinner->setValue(TO_RELATIVE(51, 0, videoQuality));
        else if (vcodec.startsWith("libvpx")) // 0 (best, 100%) - 63 (worst)
            ui->videoQualitySpinner->setValue(TO_RELATIVE(63, 0, videoQuality));
        else // 1 (best, NOT 100%) - 31 (worst)
            ui->videoQualitySpinner->setValue(TO_RELATIVE(31, 1, videoQuality));
    }
    on_audioRateControlCombo_activated(ui->audioRateControlCombo->currentIndex());
    on_videoRateControlCombo_activated(ui->videoRateControlCombo->currentIndex());
}

void EncodeDock::onProducerOpened()
{
    if (MLT.isSeekable())
        ui->encodeButton->setText(tr("Export File"));
    else
        ui->encodeButton->setText(tr("Capture File"));

    ui->fromCombo->blockSignals(true);
    ui->fromCombo->clear();
    if (MAIN.multitrack())
        ui->fromCombo->addItem(tr("Timeline"), "timeline");
    if (MAIN.playlist() && MAIN.playlist()->count() > 0)
        ui->fromCombo->addItem(tr("Playlist"), "playlist");
    if (MAIN.playlist() && MAIN.playlist()->count() > 1)
        ui->fromCombo->addItem(tr("Each Playlist Item"), "batch");
    if (MLT.isClip() && MLT.producer() && MLT.producer()->is_valid()) {
        if (MLT.producer()->get(kShotcutCaptionProperty)) {
            ui->fromCombo->addItem(MLT.producer()->get(kShotcutCaptionProperty), "clip");
        } else if (MLT.producer()->get("resource")) {
            QFileInfo resource(MLT.producer()->get("resource"));
            ui->fromCombo->addItem(resource.completeBaseName(), "clip");
        } else {
            ui->fromCombo->addItem(tr("Source"), "clip");
        }
    } else if (MLT.savedProducer() && MLT.savedProducer()->is_valid()) {
        if (MLT.savedProducer()->get(kShotcutCaptionProperty)) {
            ui->fromCombo->addItem(MLT.savedProducer()->get(kShotcutCaptionProperty), "clip");
        } else if (MLT.savedProducer()->get("resource")) {
            QFileInfo resource(MLT.savedProducer()->get("resource"));
            ui->fromCombo->addItem(resource.completeBaseName(), "clip");
        } else {
            ui->fromCombo->addItem(tr("Source"), "clip");
        }
    }
    ui->fromCombo->blockSignals(false);
}

void EncodeDock::loadPresets()
{
    QStandardItemModel* sourceModel = (QStandardItemModel*) m_presetsModel.sourceModel();
    sourceModel->clear();

    QStandardItem* parentItem = new QStandardItem(tr("Custom"));
    sourceModel->invisibleRootItem()->appendRow(parentItem);
    QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
    if (dir.cd("presets") && dir.cd("encode")) {
        QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        foreach (QString name, entries) {
            QStandardItem* item = new QStandardItem(name);
            item->setData(name);
            parentItem->appendRow(item);
        }
    }

    parentItem = new QStandardItem(tr("Stock"));
    sourceModel->invisibleRootItem()->appendRow(parentItem);
    QString prefix("consumer/avformat/");
    if (m_presets && m_presets->is_valid()) {
        for (int j = 0; j < m_presets->count(); j++) {
            QString name(m_presets->get_name(j));
            if (name.startsWith(prefix)) {
                Mlt::Properties preset((mlt_properties) m_presets->get_data(name.toLatin1().constData()));
                if (preset.get_int("meta.preset.hidden"))
                    continue;
                if (preset.get("meta.preset.name"))
                    name = QString::fromUtf8(preset.get("meta.preset.name"));
                else {
                    // use relative path and filename
                    name.remove(0, prefix.length());
                    QStringList textParts = name.split('/');
                    if (textParts.count() > 1) {
                        // if the path is a profile name, then change it to "preset (profile)"
                        QString profile = textParts.at(0);
                        textParts.removeFirst();
                        if (m_profiles->get_data(profile.toLatin1().constData()))
                            name = QString("%1 (%2)").arg(textParts.join("/")).arg(profile);
                    }
                }
                QStandardItem* item = new QStandardItem(name);
                item->setData(QString(m_presets->get_name(j)));
                if (preset.get("meta.preset.note"))
                    item->setToolTip(QString("<p>%1</p>").arg(QString::fromUtf8(preset.get("meta.preset.note"))));
                parentItem->appendRow(item);
            }
        }
    }
    m_presetsModel.sort(0);
    ui->presetsTree->expandAll();
}

Mlt::Properties* EncodeDock::collectProperties(int realtime)
{
    Mlt::Properties* p = new Mlt::Properties;
    if (p && p->is_valid()) {
        if (realtime)
            p->set("real_time", realtime);
        if (ui->formatCombo->currentIndex() != 0)
            p->set("f", ui->formatCombo->currentText().toLatin1().constData());
        if (ui->disableAudioCheckbox->isChecked()) {
            p->set("an", 1);
            p->set("audio_off", 1);
        }
        else {
            const QString& acodec = ui->audioCodecCombo->currentText();
            if (ui->audioCodecCombo->currentIndex() > 0)
                p->set("acodec", ui->audioCodecCombo->currentText().toLatin1().constData());
            p->set("ar", ui->sampleRateCombo->currentText().toLatin1().constData());
            if (ui->audioRateControlCombo->currentIndex() == RateControlAverage
                    || ui->audioRateControlCombo->currentIndex() == RateControlConstant) {
                p->set("ab", ui->audioBitrateCombo->currentText().toLatin1().constData());
                if (acodec == "libopus") {
                    if (RateControlConstant == ui->audioRateControlCombo->currentIndex())
                        p->set("vbr", "off");
                    else
                        p->set("vbr", "constrained");
                }
            } else if (acodec == "libopus") {
                p->set("vbr", "on");
                p->set("ab", ui->audioBitrateCombo->currentText().toLatin1().constData());
            } else {
                int aq = ui->audioQualitySpinner->value();
                if (acodec == "libmp3lame")
                    aq = TO_ABSOLUTE(9, 0, aq);
                else if (acodec == "libvorbis" || acodec == "vorbis")
                    aq = TO_ABSOLUTE(0, 10, aq);
                else
                    aq = TO_ABSOLUTE(0, 500, aq);
                p->set("aq", aq);
            }
        }
        if (ui->disableVideoCheckbox->isChecked()) {
            p->set("vn", 1);
            p->set("video_off", 1);
        }
        else {
            const QString& vcodec = ui->videoCodecCombo->currentText();
            if (ui->videoCodecCombo->currentIndex() > 0)
                p->set("vcodec", vcodec.toLatin1().constData());
            if (vcodec == "libx265") {
                // Most x265 parameters must be supplied through x265-params.
                QString x265params = QString::fromUtf8(p->get("x265-params"));
                switch (ui->videoRateControlCombo->currentIndex()) {
                case RateControlAverage:
                    p->set("vb", ui->videoBitrateCombo->currentText().toLatin1().constData());
                    break;
                case RateControlConstant: {
                    QString b = ui->videoBitrateCombo->currentText();
                    // x265 does not expect bitrate suffixes and requires Kb/s
                    b.replace('k', "").replace('M', "000");
                    x265params = QString("bitrate=%1:vbv-bufsize=%2:vbv-maxrate=%3:%4")
                        .arg(b).arg(int(ui->videoBufferSizeSpinner->value() * 8)).arg(b).arg(x265params);
                    break;
                    }
                case RateControlQuality: {
                    int vq = ui->videoQualitySpinner->value();
                    x265params = QString("crf=%1:%2").arg(TO_ABSOLUTE(51, 0, vq)).arg(x265params);
                    // Also set crf property so that custom presets can be interpreted properly.
                    p->set("crf", TO_ABSOLUTE(51, 0, vq));
                    break;
                    }
                }
                x265params = QString("keyint=%1:bframes=%2:%3").arg(ui->gopSpinner->value())
                            .arg(ui->bFramesSpinner->value()).arg(x265params);
                p->set("x265-params", x265params.toUtf8().constData());
            } else {
                switch (ui->videoRateControlCombo->currentIndex()) {
                case RateControlAverage:
                    p->set("vb", ui->videoBitrateCombo->currentText().toLatin1().constData());
                    break;
                case RateControlConstant: {
                    const QString& b = ui->videoBitrateCombo->currentText();
                    p->set("vb", b.toLatin1().constData());
                    p->set("vminrate", b.toLatin1().constData());
                    p->set("vmaxrate", b.toLatin1().constData());
                    p->set("vbufsize", int(ui->videoBufferSizeSpinner->value() * 8 * 1024));
                    break;
                    }
                case RateControlQuality: {
                    int vq = ui->videoQualitySpinner->value();
                    if (vcodec == "libx264") {
                        p->set("crf", TO_ABSOLUTE(51, 0, vq));
                    } else if (vcodec.startsWith("libvpx")) {
                        p->set("crf", TO_ABSOLUTE(63, 0, vq));
                        p->set("vb", 0); // VP9 needs this to prevent constrained quality mode.
                    } else {
                        p->set("qscale", TO_ABSOLUTE(31, 1, vq));
                    }
                    break;
                    }
                }
                p->set("g", ui->gopSpinner->value());
                p->set("bf", ui->bFramesSpinner->value());
            }
            p->set("width", ui->widthSpinner->value());
            p->set("height", ui->heightSpinner->value());
            p->set("aspect", double(ui->aspectNumSpinner->value()) / double(ui->aspectDenSpinner->value()));
            p->set("progressive", ui->scanModeCombo->currentIndex());
            p->set("top_field_first", ui->fieldOrderCombo->currentIndex());
            switch (ui->deinterlacerCombo->currentIndex()) {
            case 0:
                p->set("deinterlace_method", "onefield");
                break;
            case 1:
                p->set("deinterlace_method", "linearblend");
                break;
            case 2:
                p->set("deinterlace_method", "yadif-nospatial");
                break;
            default:
                p->set("deinterlace_method", "yadif");
                break;
            }
            switch (ui->interpolationCombo->currentIndex()) {
            case 0:
                p->set("rescale", "nearest");
                break;
            case 1:
                p->set("rescale", "bilinear");
                break;
            case 2:
                p->set("rescale", "bicubic");
                break;
            default:
                p->set("rescale", "hyper");
                break;
            }
            if (qFloor(ui->fpsSpinner->value() * 10.0) == 239) {
                p->set("frame_rate_num", 24000);
                p->set("frame_rate_den", 1001);
            }
            else if (qFloor(ui->fpsSpinner->value() * 10.0) == 299) {
                p->set("frame_rate_num", 30000);
                p->set("frame_rate_den", 1001);
            }
            else if (qFloor(ui->fpsSpinner->value() * 10.0) == 479) {
                p->set("frame_rate_num", 48000);
                p->set("frame_rate_den", 1001);
            }
            else if (qFloor(ui->fpsSpinner->value() * 10.0) == 599) {
                p->set("frame_rate_num", 60000);
                p->set("frame_rate_den", 1001);
            }
            else
                p->set("r", ui->fpsSpinner->value());
            if (ui->videoCodecCombo->currentText() == "prores" || ui->formatCombo->currentText() == "image2")
                p->set("threads", 1);
            else if (ui->videoCodecThreadsSpinner->value() == 0
                     && ui->videoCodecCombo->currentText() != "libx264"
                     && ui->videoCodecCombo->currentText() != "libx265")
                p->set("threads", QThread::idealThreadCount() - 1);
            else
                p->set("threads", ui->videoCodecThreadsSpinner->value());
            if (ui->dualPassCheckbox->isEnabled() && ui->dualPassCheckbox->isChecked())
                p->set("pass", 1);
        }
        foreach (QString line, ui->advancedTextEdit->toPlainText().split("\n"))
            p->parse(line.toUtf8().constData());
    }
    return p;
}

void EncodeDock::collectProperties(QDomElement& node, int realtime)
{
    Mlt::Properties* p = collectProperties(realtime);
    if (p && p->is_valid()) {
        for (int i = 0; i < p->count(); i++)
            if (p->get_name(i) && strcmp(p->get_name(i), ""))
                node.setAttribute(p->get_name(i), p->get(i));
    }
    delete p;
}

MeltJob* EncodeDock::createMeltJob(Mlt::Service* service, const QString& target, int realtime, int pass)
{
    // if image sequence, change filename to include number
    QString mytarget = target;
    if (!ui->disableVideoCheckbox->isChecked()) {
        const QString& codec = ui->videoCodecCombo->currentText();
        if (codec == "bmp" || codec == "dpx" || codec == "png" || codec == "ppm" ||
                codec == "targa" || codec == "tiff" || (codec == "mjpeg" && ui->formatCombo->currentText() == "image2")) {
            QFileInfo fi(mytarget);
            mytarget = QString("%1/%2-%05d.%3").arg(fi.path()).arg(fi.baseName()).arg(fi.completeSuffix());
        }
    }

    // get temp filename
    QTemporaryFile tmp;
    tmp.open();
    tmp.close();
    MLT.saveXML(tmp.fileName(), service);

    // parse xml
    QFile f1(tmp.fileName());
    f1.open(QIODevice::ReadOnly);
    QDomDocument dom(tmp.fileName());
    dom.setContent(&f1);
    f1.close();

    // add consumer element
    QDomElement consumerNode = dom.createElement("consumer");
    QDomNodeList profiles = dom.elementsByTagName("profile");
    if (profiles.isEmpty())
        dom.documentElement().insertAfter(consumerNode, dom.documentElement());
    else
        dom.documentElement().insertAfter(consumerNode, profiles.at(profiles.length() - 1));
    consumerNode.setAttribute("mlt_service", "avformat");
    consumerNode.setAttribute("target", mytarget);
    collectProperties(consumerNode, realtime);
    if ("libx265" == ui->videoCodecCombo->currentText()) {
        if (pass == 1 || pass == 2) {
            QString x265params = consumerNode.attribute("x265-params");
            x265params = QString("pass=%1:stats=%2:%3")
                .arg(pass).arg(mytarget.replace(":", "\\:") + "_2pass.log").arg(x265params);
            consumerNode.setAttribute("x265-params", x265params);
        }
    } else {
        if (pass == 1 || pass == 2) {
            consumerNode.setAttribute("pass", pass);
            consumerNode.setAttribute("passlogfile", mytarget + "_2pass.log");
        } if (pass == 1) {
            consumerNode.setAttribute("fastfirstpass", 1);
            consumerNode.removeAttribute("acodec");
            consumerNode.setAttribute("an", 1);
        } else {
            consumerNode.removeAttribute("fastfirstpass");
        }
    }
    if (ui->formatCombo->currentIndex() == 0 &&
            ui->audioCodecCombo->currentIndex() == 0 &&
            (mytarget.endsWith(".mp4") || mytarget.endsWith(".mov")))
        consumerNode.setAttribute("strict", "experimental");


    return new EncodeJob(target, dom.toString(2));
}

void EncodeDock::runMelt(const QString& target, int realtime)
{
    Mlt::Service* service = fromProducer();
    if (!service) {
        // For each playlist item.
        if (MAIN.playlist() && MAIN.playlist()->count() > 0) {
            // Use the first playlist item.
            QScopedPointer<Mlt::ClipInfo> info(MAIN.playlist()->clip_info(0));
            if (!info) return;
            QString xml = MLT.XML(info->producer);
            QScopedPointer<Mlt::Producer> producer(
                new Mlt::Producer(MLT.profile(), "xml-string", xml.toUtf8().constData()));
            producer->set_in_and_out(info->frame_in, info->frame_out);
            m_immediateJob = createMeltJob(producer.data(), target, realtime);
            if (m_immediateJob) {
                m_immediateJob->setIsStreaming(true);
                connect(m_immediateJob, SIGNAL(finished(AbstractJob*,bool)), this, SLOT(onFinished(AbstractJob*,bool)));
                m_immediateJob->start();
            }
            return;
        } else {
            service = MLT.producer();
        }
    }
    m_immediateJob = createMeltJob(service, target, realtime);
    if (m_immediateJob) {
        m_immediateJob->setIsStreaming(true);
        connect(m_immediateJob, SIGNAL(finished(AbstractJob*,bool)), this, SLOT(onFinished(AbstractJob*,bool)));
        m_immediateJob->start();
    }
}

void EncodeDock::enqueueMelt(const QString& target, int realtime)
{
    Mlt::Service* service = fromProducer();
    if (!service) {
        // For each playlist item.
        if (MAIN.playlist() && MAIN.playlist()->count() > 1) {
            QFileInfo fi(target);
            int n = MAIN.playlist()->count();
            int digits = QString::number(n + 1).size();
            for (int i = 0; i < n; i++) {
                QScopedPointer<Mlt::ClipInfo> info(MAIN.playlist()->clip_info(i));
                if (!info) continue;
                QString xml = MLT.XML(info->producer);
                QScopedPointer<Mlt::Producer> producer(
                    new Mlt::Producer(MLT.profile(), "xml-string", xml.toUtf8().constData()));
                producer->set_in_and_out(info->frame_in, info->frame_out);
                QString filename = QString("%1/%2-%3.%4").arg(fi.path()).arg(fi.baseName())
                                                         .arg(i + 1, digits, 10, QChar('0')).arg(fi.completeSuffix());
                int pass = ui->dualPassCheckbox->isEnabled() && ui->dualPassCheckbox->isChecked()? 1 : 0;
                MeltJob* job = createMeltJob(producer.data(), filename, realtime, pass);
                if (job) {
                    JOBS.add(job);
                    if (pass) {
                        job = createMeltJob(producer.data(), filename, realtime, 2);
                        if (job)
                            JOBS.add(job);
                    }
                }
            }
        }
    } else {
        int pass = ui->dualPassCheckbox->isEnabled() && ui->dualPassCheckbox->isChecked()? 1 : 0;
        MeltJob* job = createMeltJob(service, target, realtime, pass);
        if (job) {
            JOBS.add(job);
            if (pass) {
                job = createMeltJob(service, target, realtime, 2);
                if (job)
                    JOBS.add(job);
            }
        }
    }
}

void EncodeDock::encode(const QString& target)
{
    bool isMulti = true;
    Mlt::Producer* producer = new Mlt::Producer(MLT.producer());
    double volume = MLT.volume();
    MLT.closeConsumer();
    MLT.close();
    producer->seek(0);
    MLT.setProducer(producer, isMulti);
    MLT.consumer()->set("1", "avformat");
    MLT.consumer()->set("1.target", target.toUtf8().constData());
    Mlt::Properties* p = collectProperties(-1);
    if (p && p->is_valid()) {
        for (int i = 0; i < p->count(); i++)
            MLT.consumer()->set(QString("1.%1").arg(p->get_name(i)).toLatin1().constData(), p->get(i));
    }
    delete p;
    if (ui->formatCombo->currentIndex() == 0 &&
            ui->audioCodecCombo->currentIndex() == 0 &&
            (target.endsWith(".mp4") || target.endsWith(".mov")))
        MLT.consumer()->set("1.strict", "experimental");
    MLT.setVolume(volume);
    MLT.play();
}

void EncodeDock::resetOptions()
{
    // Reset all controls to default values.
    ui->formatCombo->setCurrentIndex(0);

    ui->widthSpinner->setEnabled(true);
    ui->heightSpinner->setEnabled(true);
    ui->aspectNumSpinner->setEnabled(true);
    ui->aspectDenSpinner->setEnabled(true);
    ui->scanModeCombo->setEnabled(true);
    ui->fpsSpinner->setEnabled(true);
    ui->deinterlacerCombo->setEnabled(true);
    ui->deinterlacerCombo->setCurrentIndex(3);
    ui->interpolationCombo->setEnabled(true);
    ui->interpolationCombo->setCurrentIndex(1);

    ui->videoBitrateCombo->lineEdit()->setText("2M");
    ui->videoBufferSizeSpinner->setValue(224);
    ui->gopSpinner->setValue(200);
    ui->bFramesSpinner->setValue(3);
    ui->videoCodecThreadsSpinner->setValue(0);
    ui->dualPassCheckbox->setChecked(false);
    ui->disableVideoCheckbox->setChecked(false);

    ui->sampleRateCombo->lineEdit()->setText("48000");
    ui->audioBitrateCombo->lineEdit()->setText("256k");
    ui->audioQualitySpinner->setValue(50);
    ui->disableAudioCheckbox->setChecked(false);

    on_videoBufferDurationChanged();

    Mlt::Properties preset;
    preset.set("f", "mp4");
    preset.set("movflags", "+faststart");
    preset.set("vcodec", "libx264");
    preset.set("crf", "21");
    preset.set("preset", "fast");
    preset.set("acodec", "aac");
    preset.set("meta.preset.extension", "mp4");
    loadPresetFromProperties(preset);
}

Mlt::Service *EncodeDock::fromProducer() const
{
    QString from = ui->fromCombo->currentData().toString();
    if (from == "clip")
        return MLT.isClip()? MLT.producer() : MLT.savedProducer();
    else if (from == "playlist")
        return MAIN.playlist();
    else if (from == "timeline")
        return MAIN.multitrack();
    else
        return 0;
}

static double getBufferSize(Mlt::Properties& preset, const char* property)
{
    double size = preset.get_double(property);
    const QString& s = preset.get(property);
    // evaluate suffix
    if (s.endsWith('k')) size *= 1000;
    if (s.endsWith('M')) size *= 1000000;
    // convert to KiB
    return double(qCeil(size / 1024 / 8 * 100)) / 100;
}

void EncodeDock::on_presetsTree_clicked(const QModelIndex &index)
{
    if (!index.parent().isValid())
        return;
    QString name = m_presetsModel.data(index, Qt::UserRole + 1).toString();
    if (!name.isEmpty()) {
        Mlt::Properties* preset;
        if (m_presetsModel.data(index.parent()).toString() == tr("Custom")) {
            ui->removePresetButton->setEnabled(true);
            preset = new Mlt::Properties();
            QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
            if (dir.cd("presets") && dir.cd("encode"))
                preset->load(dir.absoluteFilePath(name).toLatin1().constData());
        }
        else {
            ui->removePresetButton->setEnabled(false);
            preset = new Mlt::Properties((mlt_properties) m_presets->get_data(name.toLatin1().constData()));
        }
        if (preset->is_valid()) {
            QStringList textParts = name.split('/');

            resetOptions();
            if (textParts.count() > 3) {
                // textParts = ['consumer', 'avformat', profile, preset].
                QString folder = textParts.at(2);
                if (m_profiles->get_data(folder.toLatin1().constData())) {
                    // only set these fields if the folder is a profile
                    Mlt::Profile p(folder.toLatin1().constData());
                    ui->widthSpinner->setValue(p.width());
                    ui->heightSpinner->setValue(p.height());
                    ui->aspectNumSpinner->setValue(p.display_aspect_num());
                    ui->aspectDenSpinner->setValue(p.display_aspect_den());
                    ui->scanModeCombo->setCurrentIndex(p.progressive());
                    ui->fpsSpinner->setValue(p.fps());

                    ui->widthSpinner->setEnabled(false);
                    ui->heightSpinner->setEnabled(false);
                    ui->aspectNumSpinner->setEnabled(false);
                    ui->aspectDenSpinner->setEnabled(false);
                    ui->scanModeCombo->setEnabled(false);
                    ui->fpsSpinner->setEnabled(false);
                }
            }
            loadPresetFromProperties(*preset);
        }
        delete preset;
    }
}

void EncodeDock::on_presetsTree_activated(const QModelIndex &index)
{
    on_presetsTree_clicked(index);
}

void EncodeDock::on_encodeButton_clicked()
{
    if (!MLT.producer())
        return;
    if (ui->encodeButton->text() == tr("Stop Capture")) {
        MLT.closeConsumer();
        ui->encodeButton->setText(tr("Capture File"));
        emit captureStateChanged(false);
        ui->streamButton->setDisabled(false);
        QTimer::singleShot(1000, this, SLOT(openCaptureFile()));
        return;
    }
    bool seekable = MLT.isSeekable();
    QString directory = Settings.encodePath();
    if (!m_extension.isEmpty()) {
        directory += "/.";
        directory += m_extension;
    }
#ifdef Q_OS_MAC
    else {
        directory.append("/.mp4");
    }
#endif
    m_outputFilename = QFileDialog::getSaveFileName(this,
        seekable? tr("Export File") : tr("Capture File"), directory);
    if (!m_outputFilename.isEmpty()) {
        QFileInfo fi(m_outputFilename);
        MLT.pause();
        Settings.setEncodePath(fi.path());
        if (!m_extension.isEmpty()) {
            if (fi.suffix().isEmpty()) {
                m_outputFilename += '.';
                m_outputFilename += m_extension;
            }
        }
        if (seekable) {
            // Batch encode
            int threadCount = QThread::idealThreadCount();
            if (threadCount > 2 && ui->parallelCheckbox->isChecked())
                threadCount = qMin(threadCount - 1, 4);
            else
                threadCount = 1;
            enqueueMelt(m_outputFilename, Settings.playerGPU()? -1 : -threadCount);
        }
        else if (MLT.producer()->get_int(kBackgroundCaptureProperty)) {
            // Capture Shotcut screencast
            MLT.stop();
            runMelt(m_outputFilename, -1);
            ui->stopCaptureButton->show();
            if (MLT.resource().startsWith("gdigrab:"))
                MAIN.showMinimized();
        }
        else {
            // Capture to file
            // use multi consumer to encode and preview simultaneously
            ui->dualPassCheckbox->setChecked(false);
            ui->encodeButton->setText(tr("Stop Capture"));
            encode(m_outputFilename);
            emit captureStateChanged(true);
            ui->streamButton->setDisabled(true);
        }
    }
}

void EncodeDock::onProfileChanged()
{
    int width = MLT.profile().width();
    int height = MLT.profile().height();
    double sar = MLT.profile().sar();
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
    ui->widthSpinner->setValue(width);
    ui->heightSpinner->setValue(height);
    ui->aspectNumSpinner->setValue(dar_numerator);
    ui->aspectDenSpinner->setValue(dar_denominator);
    ui->scanModeCombo->setCurrentIndex(MLT.profile().progressive());
    ui->fpsSpinner->setValue(MLT.profile().fps());
}

void EncodeDock::on_streamButton_clicked()
{
    if (m_immediateJob) {
        m_immediateJob->stop();
        return;
    }
    if (ui->streamButton->text() == tr("Stop Stream")) {
        bool isMulti = false;
        MLT.closeConsumer();
        MLT.setProducer(MLT.producer(), isMulti);
        MLT.play();
        ui->streamButton->setText(tr("Stream"));
        emit captureStateChanged(false);
        emit ui->encodeButton->setDisabled(false);
        return;
    }
    QInputDialog dialog(this);
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setWindowTitle(tr("Stream"));
    dialog.setLabelText(tr("Enter the network protocol scheme, address, port, and parameters as an URL:"));
    dialog.setTextValue("udp://224.224.224.224:1234?pkt_size=1316&reuse=1");
    dialog.setWindowModality(QmlApplication::dialogModality());
    int r = dialog.exec();
    QString url = dialog.textValue();
    if (r == QDialog::Accepted && !url.isEmpty()) {
        MLT.pause();
        ui->dualPassCheckbox->setChecked(false);
        ui->streamButton->setText(tr("Stop Stream"));
        if (MLT.isSeekable())
            // Stream in background
            runMelt(url, 1);
        else if (MLT.producer()->get_int(kBackgroundCaptureProperty)) {
            // Stream Shotcut screencast
            MLT.stop();
            runMelt(url, 1);
            ui->stopCaptureButton->show();
        }
        else {
            // Live streaming in foreground
            encode(url);
            emit captureStateChanged(true);
            emit ui->encodeButton->setDisabled(true);
        }
        m_outputFilename.clear();
    }
}

void EncodeDock::on_addPresetButton_clicked()
{
    Mlt::Properties* data = collectProperties(0);
    AddEncodePresetDialog dialog(this);
    QStringList ls;

    if (data && data->is_valid())
        for (int i = 0; i < data->count(); i++)
            if (strlen(data->get_name(i)) > 0)
                ls << QString("%1=%2").arg(data->get_name(i)).arg(data->get(i));

    dialog.setWindowTitle(tr("Add Export Preset"));
    dialog.setProperties(ls.join("\n"));
    if (dialog.exec() == QDialog::Accepted) {
        QString preset = dialog.presetName();
        QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
        QString subdir("encode");

        if (!preset.isEmpty()) {
            if (!dir.exists())
                dir.mkpath(dir.path());
            if (!dir.cd("presets")) {
                if (dir.mkdir("presets"))
                    dir.cd("presets");
            }
            if (!dir.cd(subdir)) {
                if (dir.mkdir(subdir))
                    dir.cd(subdir);
            }
            QFile f(dir.filePath(preset));
            if (f.open(QIODevice::WriteOnly | QIODevice::Text))
                f.write(dialog.properties().toUtf8());

            // add the preset and select it
            loadPresets();
            QModelIndex parentIndex = m_presetsModel.index(0, 0);
            int n = m_presetsModel.rowCount(parentIndex);
            for (int i = 0; i < n; i++) {
                QModelIndex index = m_presetsModel.index(i, 0, parentIndex);
                if (m_presetsModel.data(index).toString() == preset) {
                    ui->presetsTree->setCurrentIndex(index);
                    break;
                }
            }
        }
    }
    delete data;
}

void EncodeDock::on_removePresetButton_clicked()
{
    QModelIndex index = ui->presetsTree->currentIndex();
    QString preset = m_presetsModel.data(index).toString();
    QMessageBox dialog(QMessageBox::Question,
                       tr("Delete Preset"),
                       tr("Are you sure you want to delete %1?").arg(preset),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    int result = dialog.exec();
    if (result == QMessageBox::Yes) {
        QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
        if (dir.cd("presets") && dir.cd("encode")) {
            dir.remove(preset);
            m_presetsModel.removeRow(index.row(), index.parent());
        }
    }
}

void EncodeDock::onFinished(AbstractJob* job, bool isSuccess)
{
    Q_UNUSED(isSuccess)
    if (!MLT.isSeekable())
        ui->encodeButton->setText(tr("Capture File"));
    ui->streamButton->setText(tr("Stream"));
    m_immediateJob = 0;
    delete job;
    emit captureStateChanged(false);
    ui->encodeButton->setDisabled(false);
}

void EncodeDock::on_stopCaptureButton_clicked()
{
    ui->stopCaptureButton->hide();
    if (m_immediateJob)
        m_immediateJob->stop();
    if (!m_outputFilename.isEmpty())
        QTimer::singleShot(1000, this, SLOT(openCaptureFile()));
}

void EncodeDock::on_videoRateControlCombo_activated(int index)
{
    switch (index) {
    case RateControlAverage:
        ui->videoBitrateCombo->show();
        ui->videoBufferSizeSpinner->hide();
        ui->videoQualitySpinner->hide();
        ui->dualPassCheckbox->show();
        ui->videoBitrateLabel->show();
        ui->videoBitrateSuffixLabel->show();
        ui->videoBufferSizeLabel->hide();
        ui->videoBufferSizeSuffixLabel->hide();
        ui->videoQualityLabel->hide();
        break;
    case RateControlConstant:
        ui->videoBitrateCombo->show();
        ui->videoBufferSizeSpinner->show();
        ui->videoQualitySpinner->hide();
        ui->dualPassCheckbox->show();
        ui->videoBitrateLabel->show();
        ui->videoBitrateSuffixLabel->show();
        ui->videoBufferSizeLabel->show();
        ui->videoBufferSizeSuffixLabel->show();
        ui->videoQualityLabel->hide();
        break;
    case RateControlQuality:
        ui->videoBitrateCombo->hide();
        ui->videoBufferSizeSpinner->hide();
        ui->videoQualitySpinner->show();
        ui->dualPassCheckbox->hide();
        ui->videoBitrateLabel->hide();
        ui->videoBitrateSuffixLabel->hide();
        ui->videoBufferSizeLabel->hide();
        ui->videoBufferSizeSuffixLabel->hide();
        ui->videoQualityLabel->show();
        break;
    }
}

void EncodeDock::on_audioRateControlCombo_activated(int index)
{
    if (ui->audioCodecCombo->currentText() == "libopus")
        // libopus does not use % for quality
        return;
    switch (index) {
    case RateControlAverage:
        ui->audioBitrateCombo->show();
        ui->audioQualitySpinner->hide();
        ui->audioBitrateLabel->show();
        ui->audioBitrateSuffixLabel->show();
        ui->audioQualityLabel->hide();
        break;
    case RateControlConstant:
        ui->audioBitrateCombo->show();
        ui->audioQualitySpinner->hide();
        ui->audioBitrateLabel->show();
        ui->audioBitrateSuffixLabel->show();
        ui->audioQualityLabel->hide();
        break;
    case RateControlQuality:
        ui->audioBitrateCombo->hide();
        ui->audioQualitySpinner->show();
        ui->audioBitrateLabel->hide();
        ui->audioBitrateSuffixLabel->hide();
        ui->audioQualityLabel->show();
        break;
    }
}

void EncodeDock::on_scanModeCombo_currentIndexChanged(int index)
{
    ui->fieldOrderCombo->setDisabled(index);
    ui->deinterlacerCombo->setEnabled(index);
}

void EncodeDock::on_presetsSearch_textChanged(const QString &search)
{
    m_presetsModel.setFilterFixedString(search);
}

bool PresetsProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    return !source_parent.isValid() || 
        sourceModel()->data(index).toString().contains(filterRegExp()) ||
        sourceModel()->data(index, Qt::ToolTipRole).toString().contains(filterRegExp());
}

void EncodeDock::on_resetButton_clicked()
{
    resetOptions();
    onProfileChanged();
}

void EncodeDock::openCaptureFile()
{
    MAIN.open(m_outputFilename);
}

void EncodeDock::on_formatCombo_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    m_extension.clear();
}

void EncodeDock::on_videoBufferDurationChanged()
{
    QString vb = ui->videoBitrateCombo->currentText();
    vb.replace('k', "").replace('M', "000");
    double duration = (double)ui->videoBufferSizeSpinner->value() * 8.0 / vb.toDouble();
    QString label = QString(tr("KiB (%1s)")).arg(duration);
    ui->videoBufferSizeSuffixLabel->setText(label);
}
