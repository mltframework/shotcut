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

#include "encodedock.h"
#include "ui_encodedock.h"
#include "dialogs/addencodepresetdialog.h"
#include "jobqueue.h"
#include "mltcontroller.h"
#include "mainwindow.h"
#include <QtDebug>
#include <QtWidgets>
#include <QtXml>

// formulas to map absolute value ranges to percentages as int
#define TO_ABSOLUTE(min, max, rel) ((min) + ((max) - (min)) * (rel) / 100)
#define TO_RELATIVE(min, max, abs) (100 * ((abs) - (min)) / ((max) - (min)))

EncodeDock::EncodeDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::EncodeDock),
    m_presets(Mlt::Repository::presets()),
    m_immediateJob(0),
    m_profiles(Mlt::Profile::list())
{
    ui->setupUi(this);
#ifdef Q_OS_UNIX
    ui->stopCaptureButton->hide();
#else
    delete ui->stopCaptureButton;
#endif
    ui->videoCodecThreadsSpinner->setMaximum(QThread::idealThreadCount());
    toggleViewAction()->setIcon(QIcon::fromTheme("media-record", windowIcon()));
    ui->addPresetButton->setIcon(QIcon::fromTheme("list-add", ui->addPresetButton->icon()));
    ui->removePresetButton->setIcon(QIcon::fromTheme("list-remove", ui->removePresetButton->icon()));

#ifdef Q_OS_WIN
    // Add splitter handle decoration for Windows.
    QSplitterHandle *handle = ui->splitter->handle(1);
    QHBoxLayout *layout = new QHBoxLayout(handle);
    QVBoxLayout *vbox = new QVBoxLayout;
    ui->splitter->setHandleWidth(7);
    handle->setLayout(layout);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addStretch();
    layout->addLayout(vbox);
    for (int i = 0; i < 2; i++) {
        QFrame *line = new QFrame(handle);
        line->setMinimumSize(15, 1);
        line->setMaximumSize(15, 1);
        line->setFrameShape(QFrame::StyledPanel);
        vbox->addWidget(line);
    }
    layout->addStretch();
#endif

    loadPresets();

    // populate the combos
    Mlt::Consumer c(MLT.profile(), "avformat");
    c.set("f", "list");
    c.set("acodec", "list");
    c.set("vcodec", "list");
    c.start();
    c.stop();

    Mlt::Properties* p = new Mlt::Properties(c.get_data("f"));
    ui->formatCombo->addItem(tr("Automatic from extension"));
    for (int i = 0; i < p->count(); i++)
        ui->formatCombo->addItem(p->get(i));
    delete p;
    ui->formatCombo->model()->sort(0);

    p = new Mlt::Properties(c.get_data("acodec"));
    ui->audioCodecCombo->addItem(tr("Default for format"));
    for (int i = 0; i < p->count(); i++)
        ui->audioCodecCombo->addItem(p->get(i));
    delete p;
    ui->audioCodecCombo->model()->sort(0);

    p = new Mlt::Properties(c.get_data("vcodec"));
    ui->videoCodecCombo->addItem(tr("Default for format"));
    for (int i = 0; i < p->count(); i++)
        ui->videoCodecCombo->addItem(p->get(i));
    delete p;
    ui->videoCodecCombo->model()->sort(0);
}

EncodeDock::~EncodeDock()
{
    delete ui;
    delete m_presets;
    delete m_profiles;
}

void EncodeDock::onProducerOpened()
{
    if (MLT.isSeekable())
        ui->encodeButton->setText(tr("Encode File"));
    else
        ui->encodeButton->setText(tr("Capture File"));
}

void EncodeDock::loadPresets()
{
    ui->presetsTree->clear();
    QTreeWidgetItem* item = new QTreeWidgetItem(QStringList(tr("Custom")));
    item->setFlags(Qt::ItemIsEnabled);
    ui->presetsTree->addTopLevelItem(item);
    item = new QTreeWidgetItem(QStringList(tr("Stock")));
    item->setFlags(Qt::ItemIsEnabled);
    ui->presetsTree->addTopLevelItem(item);
    QString prefix("consumer/avformat/");
    for (int i = 0; i < ui->presetsTree->topLevelItemCount(); i++) {
        QTreeWidgetItem* group = ui->presetsTree->topLevelItem(i);
        if (group->text(0) == tr("Stock")) {
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
                        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(name));
                        item->setData(0, Qt::UserRole, QString(m_presets->get_name(j)));
                        if (preset.get("meta.preset.note"))
                            item->setToolTip(0, QString("<p>%1</p>").arg(QString::fromUtf8(preset.get("meta.preset.note"))));
                    }
                }
            }
        }
        else if (group->text(0) == tr("Custom")) {
            QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
            if (dir.cd("presets") && dir.cd("encode")) {
                QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
                foreach (QString name, entries) {
                    QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(name));
                    item->setData(0, Qt::UserRole, name);
                }
            }
        }
    }
    ui->presetsTree->model()->sort(0);
    ui->presetsTree->expandAll();
}

Mlt::Properties* EncodeDock::collectProperties(int realtime)
{
    Mlt::Properties* p = new Mlt::Properties;
    if (p && p->is_valid()) {
        if (realtime)
            p->set("real_time", realtime);
        if (ui->formatCombo->currentText() != tr("Automatic from extension"))
            p->set("f", ui->formatCombo->currentText().toLatin1().constData());
        if (ui->disableAudioCheckbox->isChecked()) {
            p->set("an", 1);
            p->set("audio_off", 1);
        }
        else {
            if (ui->audioCodecCombo->currentIndex() > 0)
                p->set("acodec", ui->audioCodecCombo->currentText().toLatin1().constData());
            p->set("ar", ui->sampleRateCombo->currentText().toLatin1().constData());
            if (ui->audioRateControlCombo->currentIndex() == RateControlAverage
                    || ui->audioRateControlCombo->currentIndex() == RateControlConstant) {
                p->set("ab", ui->audioBitrateCombo->currentText().toLatin1().constData());
            }
            else {
                const QString& acodec = ui->audioCodecCombo->currentText();
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
            if (ui->videoCodecCombo->currentIndex() > 0)
                p->set("vcodec", ui->videoCodecCombo->currentText().toLatin1().constData());
            if (ui->videoRateControlCombo->currentIndex() == RateControlAverage) {
                p->set("vb", ui->videoBitrateCombo->currentText().toLatin1().constData());
            }
            else if (ui->videoRateControlCombo->currentIndex() == RateControlConstant) {
                const QString& b = ui->videoBitrateCombo->currentText();
                p->set("vb", b.toLatin1().constData());
                p->set("vminrate", b.toLatin1().constData());
                p->set("vmaxrate", b.toLatin1().constData());
                p->set("vbufsize", int(ui->videoBufferSizeSpinner->value() * 8 * 1024));
            }
            else { // RateControlQuality
                const QString& vcodec = ui->videoCodecCombo->currentText();
                int vq = ui->videoQualitySpinner->value();
                if (vcodec == "libx264")
                    p->set("crf", TO_ABSOLUTE(51, 0, vq));
                else
                    p->set("qscale", TO_ABSOLUTE(31, 1, vq));
            }
            p->set("g", ui->gopSpinner->value());
            p->set("bf", ui->bFramesSpinner->value());
            p->set("width", ui->widthSpinner->value());
            p->set("height", ui->heightSpinner->value());
            p->set("aspect", QString("@%1/%2").arg(ui->aspectNumSpinner->value()).arg(ui->aspectDenSpinner->value()).toLatin1().constData());
            p->set("progressive", ui->scanModeCombo->currentIndex());
            p->set("top_field_first", ui->fieldOrderCombo->currentIndex());
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
            if (ui->videoCodecCombo->currentText() == "prores")
                p->set("threads", 1);
            else if (ui->videoCodecThreadsSpinner->value() == 0 && ui->videoCodecCombo->currentText() != "libx264")
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

MeltJob* EncodeDock::createMeltJob(const QString& target, int realtime, int pass)
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
    QTemporaryFile tmp(QDir::tempPath().append("/shotcut-XXXXXX"));
    tmp.open();
    QString tmpName = tmp.fileName();
    tmp.close();
    tmpName.append(".mlt");
    MAIN.saveXML(tmpName);

    // parse xml
    QFile f1(tmpName);
    f1.open(QIODevice::ReadOnly);
    QDomDocument dom(tmpName);
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
    if (pass == 1 || pass == 2)
        consumerNode.setAttribute("pass", pass);
    if (pass == 1) {
        consumerNode.setAttribute("fastfirstpass", 1);
        consumerNode.removeAttribute("acodec");
        consumerNode.setAttribute("an", 1);
    }
    else
        consumerNode.removeAttribute("fastfirstpass");

    // save new xml
    f1.open(QIODevice::WriteOnly);
    QTextStream ts(&f1);
    dom.save(ts, 2);
    f1.close();

    return new MeltJob(target, tmpName);
}

void EncodeDock::runMelt(const QString& target, int realtime)
{
    m_immediateJob = createMeltJob(target, realtime);
    if (m_immediateJob) {
        connect(m_immediateJob, SIGNAL(finished(MeltJob*,bool)), this, SLOT(onFinished(MeltJob*,bool)));
        m_immediateJob->start();
    }
}

void EncodeDock::enqueueMelt(const QString& target, int realtime)
{
    int pass = ui->dualPassCheckbox->isEnabled() && ui->dualPassCheckbox->isChecked()? 1 : 0;
    MeltJob* job = createMeltJob(target, realtime, pass);
    if (job) {
        JOBS.add(job);
        if (pass) {
            job = createMeltJob(target, realtime, 2);
            if (job)
                JOBS.add(job);
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
    MLT.open(producer, isMulti);
    MLT.consumer()->set("1", "avformat");
    MLT.consumer()->set("1.target", target.toUtf8().constData());
    Mlt::Properties* p = collectProperties(-1);
    if (p && p->is_valid()) {
        for (int i = 0; i < p->count(); i++)
            MLT.consumer()->set(QString("1.%1").arg(p->get_name(i)).toLatin1().constData(), p->get(i));
    }
    delete p;
    MLT.setVolume(volume);
    MLT.play();
}

static double getBufferSize(Mlt::Properties* preset, const char* property)
{
    double size = preset->get_double(property);
    const QString& s = preset->get(property);
    // evaluate suffix
    if (s.endsWith('k')) size *= 1000;
    if (s.endsWith('M')) size *= 1000000;
    // convert to KiB
    return size / 1024 / 8;
}

void EncodeDock::on_presetsTree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (!current || !current->parent())
        return;
    QString name = current->data(0, Qt::UserRole).toString();
    if (!name.isEmpty()) {
        Mlt::Properties* preset;
        if (current->parent()->text(0) == tr("Custom")) {
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
            int audioQuality;
            int videoQuality;
            QStringList other;
            QStringList textParts = name.split('/');

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
                }
            }
            ui->disableAudioCheckbox->setChecked(preset->get_int("an"));
            ui->disableVideoCheckbox->setChecked(preset->get_int("vn"));
            m_extension.clear();
            for (int i = 0; i < preset->count(); i++) {
                QString name(preset->get_name(i));
                if (name == "f") {
                    for (int i = 0; i < ui->formatCombo->count(); i++)
                        if (ui->formatCombo->itemText(i) == preset->get("f"))
                            ui->formatCombo->setCurrentIndex(i);
                }
                else if (name == "acodec") {
                    for (int i = 0; i < ui->audioCodecCombo->count(); i++)
                        if (ui->audioCodecCombo->itemText(i) == preset->get("acodec"))
                            ui->audioCodecCombo->setCurrentIndex(i);
                }
                else if (name == "vcodec") {
                    for (int i = 0; i < ui->videoCodecCombo->count(); i++)
                        if (ui->videoCodecCombo->itemText(i) == preset->get("vcodec"))
                            ui->videoCodecCombo->setCurrentIndex(i);
                }
                else if (name == "ar")
                    ui->sampleRateCombo->lineEdit()->setText(preset->get("ar"));
                else if (name == "ab")
                    ui->audioBitrateCombo->lineEdit()->setText(preset->get("ab"));
                else if (name == "vb")
                    ui->videoBitrateCombo->lineEdit()->setText(preset->get("vb"));
                else if (name == "g")
                    ui->gopSpinner->setValue(preset->get_int("g"));
                else if (name == "bf")
                    ui->bFramesSpinner->setValue(preset->get_int("bf"));
                else if (name == "deinterlace")
                    ui->scanModeCombo->setCurrentIndex(preset->get_int("deinterlace"));
                else if (name == "progressive")
                    ui->scanModeCombo->setCurrentIndex(preset->get_int("progressive"));
                else if (name == "top_field_first")
                    ui->fieldOrderCombo->setCurrentIndex(preset->get_int("top_field_first"));
                else if (name == "width")
                    ui->widthSpinner->setValue(preset->get_int("width"));
                else if (name == "height")
                    ui->heightSpinner->setValue(preset->get_int("height"));
                else if (name == "aspect") {
                    double dar = preset->get_double("aspect");
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
                }
                else if (name == "r")
                    ui->fpsSpinner->setValue(preset->get_double("r"));
                else if (name == "pix_fmt") {
                    QString pix_fmt(preset->get("pix_fmt"));
                    if (pix_fmt != "yuv420p")
                        other.append(QString("%1=%2").arg(name).arg(pix_fmt));
                }
                else if (name == "pass")
                    ui->dualPassCheckbox->setChecked(true);
                else if (name == "aq") {
                    ui->audioRateControlCombo->setCurrentIndex(RateControlQuality);
                    audioQuality = preset->get_int("aq");
                }
                else if (name == "vq") {
                    ui->videoRateControlCombo->setCurrentIndex(RateControlQuality);
                    videoQuality = preset->get_int("vq");
                }
                else if (name == "qscale") {
                    ui->videoRateControlCombo->setCurrentIndex(RateControlQuality);
                    videoQuality = preset->get_int("qscale");
                }
                else if (name == "crf") {
                    ui->videoRateControlCombo->setCurrentIndex(RateControlQuality);
                    ui->videoQualitySpinner->setValue(preset->get_int("crf"));
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
                    if (preset->get_int("threads") == 1)
                        ui->videoCodecThreadsSpinner->setValue(1);
                }
                else if (name == "meta.preset.extension")
                    m_extension = preset->get("meta.preset.extension");
                else if (name != "an" && name != "vn" && name != "threads"
                         && !name.startsWith("meta.preset."))
                    other.append(QString("%1=%2").arg(name).arg(preset->get(i)));
            }
            ui->advancedTextEdit->setPlainText(other.join("\n"));

            // normalize the quality settings
            // quality depends on codec
            if (ui->audioRateControlCombo->currentIndex() == RateControlQuality) {
                const QString& acodec = ui->audioCodecCombo->currentText();
                if (acodec == "libmp3lame") // 0 (best) - 9 (worst)
                    ui->audioQualitySpinner->setValue(TO_RELATIVE(9, 0, audioQuality));
                if (acodec == "libvorbis" || acodec == "vorbis") // 0 (worst) - 10 (best)
                    ui->audioQualitySpinner->setValue(TO_RELATIVE(0, 10, audioQuality));
                else
                    // aac: 0 (worst) - 500 (best)
                    ui->audioQualitySpinner->setValue(TO_RELATIVE(0, 500, audioQuality));
            }
            if (ui->videoRateControlCombo->currentIndex() == RateControlQuality) {
                const QString& vcodec = ui->videoCodecCombo->currentText();
                //val = min + (max - min) * paramval;
                if (vcodec == "libx264") // 0 (best, 100%) -51 (worst)
                    ui->videoQualitySpinner->setValue(TO_RELATIVE(51, 0, videoQuality));
                else // 1 (best, NOT 100%) - 31 (worst)
                    ui->videoQualitySpinner->setValue(TO_RELATIVE(31, 1, videoQuality));
            }
            on_audioRateControlCombo_activated(ui->audioRateControlCombo->currentIndex());
            on_videoRateControlCombo_activated(ui->videoRateControlCombo->currentIndex());
        }
        delete preset;
    }
}

void EncodeDock::on_encodeButton_clicked()
{
    if (!MLT.producer())
        return;
    if (ui->encodeButton->text() == tr("Stop Capture")) {
        bool isMulti = false;
        MLT.closeConsumer();
        MLT.open(MLT.producer(), isMulti);
        MLT.play();
        ui->encodeButton->setText(tr("Capture File"));
        emit captureStateChanged(false);
        ui->streamButton->setDisabled(false);
        return;
    }
    bool seekable = MLT.isSeekable();
    QSettings settings;
    QString settingKey("encode/path");
    QString directory(settings.value(settingKey,
        QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString());
    if (!m_extension.isEmpty()) {
        directory += "/.";
        directory += m_extension;
    }
    QString outputFilename = QFileDialog::getSaveFileName(this,
        seekable? tr("Encode to File") : tr("Capture to File"), directory);
    if (!outputFilename.isEmpty()) {
        QFileInfo fi(outputFilename);
        MLT.pause();
        settings.setValue(settingKey, fi.path());
        if (!m_extension.isEmpty()) {
            if (fi.suffix().isEmpty()) {
                outputFilename += '.';
                outputFilename += m_extension;
            }
        }
        if (seekable)
            // Batch encode
            enqueueMelt(outputFilename);
        else if (MLT.producer()->get_int("shotcut_bgcapture")) {
            MLT.stop();
            runMelt(outputFilename);
            ui->stopCaptureButton->show();
        }
        else {
            // Capture to file
            // use multi consumer to encode and preview simultaneously
            ui->dualPassCheckbox->setChecked(false);
            ui->encodeButton->setText(tr("Stop Capture"));
            encode(outputFilename);
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
        MLT.open(MLT.producer(), isMulti);
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
    dialog.setWindowModality(Qt::WindowModal);
    int r = dialog.exec();
    QString url = dialog.textValue();
    if (r == QDialog::Accepted && !url.isEmpty()) {
        MLT.pause();
        ui->dualPassCheckbox->setChecked(false);
        ui->streamButton->setText(tr("Stop Stream"));
        if (MLT.isSeekable())
            // Stream in background
            runMelt(url, 1);
        else if (MLT.producer()->get_int("shotcut_bgcapture")) {
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

    dialog.setWindowTitle(tr("Add Encode Preset"));
    dialog.setProperties(ls.join("\n"));
    if (dialog.exec() == QDialog::Accepted) {
        QString preset = dialog.presetName();
        QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
        QString subdir("encode");

        if (!preset.isEmpty()) {
            qDebug() << dialog.properties();
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
            for (int i = 0; i < ui->presetsTree->topLevelItemCount(); i++) {
                QTreeWidgetItem* group = ui->presetsTree->topLevelItem(i);
                if (group->text(0) == tr("Custom")) {
                    for (int j = 0; j < group->childCount(); j++) {
                        if (group->child(j)->text(0) == preset) {
                            ui->presetsTree->setCurrentItem(group->child(j), 0);
                            break;
                        }
                    }
                }
            }
        }
    }
    delete data;
}

void EncodeDock::on_removePresetButton_clicked()
{
    QString preset(ui->presetsTree->currentItem()->text(0));
    QMessageBox dialog(QMessageBox::Question,
                       tr("Delete Preset"),
                       tr("Are you sure you want to delete") + " " + preset + "?",
                       QMessageBox::No | QMessageBox::Yes,
                       this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(Qt::WindowModal);
    int result = dialog.exec();
    if (result == QMessageBox::Yes) {
        QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
        if (dir.cd("presets") && dir.cd("encode")) {
            dir.remove(preset);
            ui->presetsTree->topLevelItem(0)->removeChild(ui->presetsTree->currentItem());
        }
    }
}

void EncodeDock::onFinished(MeltJob* job, bool isSuccess)
{
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
}

void EncodeDock::on_videoRateControlCombo_activated(int index)
{
    switch (index) {
    case RateControlAverage:
        ui->videoBitrateCombo->setEnabled(true);
        ui->videoBufferSizeSpinner->setEnabled(false);
        ui->videoQualitySpinner->setEnabled(false);
        ui->dualPassCheckbox->setEnabled(true);
        break;
    case RateControlConstant:
        ui->videoBitrateCombo->setEnabled(true);
        ui->videoBufferSizeSpinner->setEnabled(true);
        ui->videoQualitySpinner->setEnabled(false);
        ui->dualPassCheckbox->setEnabled(false);
        break;
    case RateControlQuality:
        ui->videoBitrateCombo->setEnabled(false);
        ui->videoBufferSizeSpinner->setEnabled(false);
        ui->videoQualitySpinner->setEnabled(true);
        ui->dualPassCheckbox->setEnabled(false);
        break;
    }
}

void EncodeDock::on_audioRateControlCombo_activated(int index)
{
    switch (index) {
    case RateControlAverage:
        ui->audioBitrateCombo->setEnabled(true);
        ui->audioQualitySpinner->setEnabled(false);
        break;
    case RateControlConstant:
        ui->audioBitrateCombo->setEnabled(true);
        ui->audioQualitySpinner->setEnabled(false);
        break;
    case RateControlQuality:
        ui->audioBitrateCombo->setEnabled(false);
        ui->audioQualitySpinner->setEnabled(true);
        break;
    }
}

void EncodeDock::on_scanModeCombo_currentIndexChanged(int index)
{
    ui->fieldOrderCombo->setDisabled(index);
}
