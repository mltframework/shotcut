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
#include <mltcontroller.h>
#include <QtDebug>
#include <QtGui>
#include <QtXml>

EncodeDock::EncodeDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::EncodeDock),
    m_presets(Mlt::Repository::presets()),
    m_immediateJob(0)
{
    ui->setupUi(this);
    ui->videoCodecThreadsSpinner->setMaximum(QThread::idealThreadCount());
//    toggleViewAction()->setIcon(QIcon::fromTheme("media-record", windowIcon()));
    ui->addPresetButton->setIcon(QIcon::fromTheme("list-add", ui->addPresetButton->icon()));
    ui->removePresetButton->setIcon(QIcon::fromTheme("list-remove", ui->removePresetButton->icon()));
    ui->reloadSignalButton->setIcon(QIcon::fromTheme("view-refresh", ui->reloadSignalButton->icon()));

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
    p = new Mlt::Properties(c.get_data("acodec"));
    ui->audioCodecCombo->addItem(tr("Default for format"));
    for (int i = 0; i < p->count(); i++)
        ui->audioCodecCombo->addItem(p->get(i));
    delete p;
    p = new Mlt::Properties(c.get_data("vcodec"));
    ui->videoCodecCombo->addItem(tr("Default for format"));
    for (int i = 0; i < p->count(); i++)
        ui->videoCodecCombo->addItem(p->get(i));
    delete p;
}

EncodeDock::~EncodeDock()
{
    delete ui;
    delete m_presets;
}

void EncodeDock::onProducerOpened()
{
    if (MLT.producer()->get_int("seekable"))
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
                        name.remove(0, prefix.length());
                        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(name));
                        item->setData(0, Qt::UserRole, QString(m_presets->get_name(j)));
                    }
                }
            }
        }
        else if (group->text(0) == tr("Custom")) {
            QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
            if (dir.cd("presets") && dir.cd("encode")) {
                QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
                foreach (QString name, entries) {
                    QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(name));
                    item->setData(0, Qt::UserRole, name);
                }
            }
        }
    }

    ui->presetsTree->expandAll();
}

Mlt::Properties* EncodeDock::collectProperties(int realtime)
{
    Mlt::Properties* p = new Mlt::Properties;
    if (p && p->is_valid()) {
        if (realtime)
            p->set("real_time", realtime);
        if (ui->formatCombo->currentIndex() > 0)
            p->set("f", ui->formatCombo->currentText().toAscii().constData());
        if (ui->disableAudioCheckbox->isChecked())
            p->set("an", 1);
        else {
            if (ui->audioCodecCombo->currentIndex() > 0)
                p->set("acodec", ui->audioCodecCombo->currentText().toAscii().constData());
            p->set("ar", ui->sampleRateCombo->currentText().toAscii().constData());
            p->set("ab", ui->audioBitrateCombo->currentText().toAscii().constData());
        }
        if (ui->disableVideoCheckbox->isChecked())
            p->set("vn", 1);
        else {
            if (ui->videoCodecCombo->currentIndex() > 0)
                p->set("vcodec", ui->videoCodecCombo->currentText().toAscii().constData());
            p->set("vb", ui->videoBitrateCombo->currentText().toAscii().constData());
            p->set("g", ui->gopSpinner->value());
            p->set("bf", ui->bFramesSpinner->value());
            p->set("width", ui->widthSpinner->value());
            p->set("height", ui->heightSpinner->value());
            p->set("aspect", QString("@%1/%2").arg(ui->aspectNumSpinner->value()).arg(ui->aspectDenSpinner->value()).toAscii().constData());
            p->set("progressive", ui->scanModeCombo->currentIndex());
            p->set("top_field_first", ui->fieldOrderCombo->currentIndex());
            p->set("r", ui->fpsSpinner->value());
            if (ui->videoCodecThreadsSpinner->value() == 0 && ui->videoCodecCombo->currentText() != "libx264")
                p->set("threads", QThread::idealThreadCount() - 1);
            else
                p->set("threads", ui->videoCodecThreadsSpinner->value());
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
            node.setAttribute(p->get_name(i), p->get(i));
    }
    delete p;
}

MeltJob* EncodeDock::createMeltJob(const QString& target, int realtime)
{
    // get temp filename
    QTemporaryFile tmp(QDir::tempPath().append("/shotcut-XXXXXX"));
    tmp.open();
    QString tmpName = tmp.fileName();
    tmp.close();
    tmpName.append(".mlt");
    MLT.saveXML(tmpName);

    // parse xml
    QFile f1(tmpName);
    f1.open(QIODevice::ReadOnly);
    QDomDocument dom(tmpName);
    dom.setContent(&f1);
    f1.close();

    // add consumer element
    QDomElement consumerNode = dom.createElement("consumer");
    dom.documentElement().appendChild(consumerNode);
    consumerNode.setAttribute("mlt_service", "avformat");
    consumerNode.setAttribute("target", target);
    collectProperties(consumerNode, realtime);

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
    MeltJob* job = createMeltJob(target, realtime);
    if (job)
        JOBS.add(job);
}

void EncodeDock::encode(const QString& target)
{
    bool isMulti = true;
    Mlt::Producer* producer = new Mlt::Producer(MLT.producer());
    double volume = MLT.volume();
    MLT.close();
    producer->seek(0);
    MLT.open(producer, isMulti);
    MLT.consumer()->set("1", "avformat");
    MLT.consumer()->set("1.target", target.toUtf8().constData());
    Mlt::Properties* p = collectProperties(-1);
    if (p && p->is_valid()) {
        for (int i = 0; i < p->count(); i++)
            MLT.consumer()->set(QString("1.%1").arg(p->get_name(i)).toAscii().constData(), p->get(i));
    }
    delete p;
    MLT.setVolume(volume);
    MLT.play();
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
            QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
            if (dir.cd("presets") && dir.cd("encode"))
                preset->load(dir.absoluteFilePath(name).toAscii().constData());
        }
        else {
            ui->removePresetButton->setEnabled(false);
            preset = new Mlt::Properties((mlt_properties) m_presets->get_data(name.toAscii().constData()));
        }
        if (preset->is_valid()) {
            QStringList other;
            QStringList textParts = current->text(0).split('/');
            if (textParts.count() > 1) {
                Mlt::Profile p(textParts.at(0).toAscii().constData());
                ui->widthSpinner->setValue(p.width());
                ui->heightSpinner->setValue(p.height());
                ui->aspectNumSpinner->setValue(p.display_aspect_num());
                ui->aspectDenSpinner->setValue(p.display_aspect_den());
                ui->scanModeCombo->setCurrentIndex(p.progressive());
                ui->fpsSpinner->setValue(p.fps());
            }
//            ui->formatCombo->setCurrentIndex(0);
//            ui->audioCodecCombo->setCurrentIndex(0);
            ui->disableAudioCheckbox->setChecked(preset->get_int("an"));
//            ui->videoCodecCombo->setCurrentIndex(0);
            ui->disableVideoCheckbox->setChecked(preset->get_int("vn"));
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
                else if (name != "an" && name != "vn" && name != "threads")
                    other.append(QString("%1=%2").arg(name).arg(preset->get(i)));
            }
            ui->advancedTextEdit->setPlainText(other.join("\n"));
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
        MLT.open(MLT.producer(), isMulti);
        ui->encodeButton->setText(tr("Capture File"));
        emit captureStateChanged(false);
        ui->streamButton->setDisabled(false);
        return;
    }
    bool seekable = MLT.producer()->get_int("seekable");
    QSettings settings;
    QString settingKey("encode/path");
    QString directory(settings.value(settingKey,
        QDesktopServices::storageLocation(QDesktopServices::MoviesLocation)).toString());
    QString outputFilename = QFileDialog::getSaveFileName(this,
        seekable? tr("Encode to File") : tr("Capture to File"), directory);
    if (!outputFilename.isEmpty()) {
        MLT.pause();
        settings.setValue(settingKey, QFileInfo(outputFilename).path());
        if (seekable)
            enqueueMelt(outputFilename);
        else {
            // use multi consumer to encode and preview simultaneously
            ui->encodeButton->setText(tr("Stop Capture"));
            encode(outputFilename);
            emit captureStateChanged(true);
            ui->streamButton->setDisabled(true);
        }
    }
}

void EncodeDock::on_reloadSignalButton_clicked()
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
        m_immediateJob->kill();
        return;
    }
    if (ui->streamButton->text() == tr("Stop Stream")) {
        bool isMulti = false;
        MLT.open(MLT.producer(), isMulti);
        ui->streamButton->setText(tr("Stream"));
        return;
    }
    QString url = QInputDialog::getText(this, tr("Stream"),
        tr("Enter the network protocol scheme, address, port, and parameters as an URL:"),
        QLineEdit::Normal, "udp://224.224.224.224:1234?pkt_size=1316&reuse=1");
    if (!url.isEmpty()) {
        MLT.pause();
        ui->dualPassCheckbox->setChecked(false);
        ui->streamButton->setText(tr("Stop Stream"));
        if (MLT.producer()->get_int("seekable"))
            runMelt(url, 1);
        else {
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
        QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
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
    int result = QMessageBox::question(this, tr("Delete Preset"),
                                       tr("Are you sure you want to delete") + " " + preset + "?",
                                       QMessageBox::No | QMessageBox::Yes, QMessageBox::No);
    if (result == QMessageBox::Yes) {
        QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
        if (dir.cd("presets") && dir.cd("encode")) {
            dir.remove(preset);
            ui->presetsTree->topLevelItem(0)->removeChild(ui->presetsTree->currentItem());
        }
    }
}

void EncodeDock::onFinished(MeltJob* job, bool isSuccess)
{
    if (!MLT.producer()->get_int("seekable"))
        ui->encodeButton->setText(tr("Capture File"));
    ui->streamButton->setText(tr("Stream"));
    m_immediateJob = 0;
    delete job;
    emit captureStateChanged(false);
    ui->encodeButton->setDisabled(false);
}
