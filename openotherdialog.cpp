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

#include "openotherdialog.h"
#include "ui_openotherdialog.h"
#include "mltcontroller.h"
#include <Mlt.h>
#include <QtGui>

OpenOtherDialog::OpenOtherDialog(Mlt::Controller *mc, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OpenOtherDialog),
    mlt(mc)
{
    ui->setupUi(this);

    QTreeWidgetItem* group = new QTreeWidgetItem(ui->treeWidget, QStringList(tr("Network")));
    group->setData(0, Qt::UserRole, 0);
    ui->treeWidget->setCurrentItem(group);

    saveDefaultPreset("color");
    saveDefaultPreset("frei0r.ising0r");
    saveDefaultPreset("frei0r.lissajous0r");
    saveDefaultPreset("frei0r.plasma");
    loadPresets();

    // populate the device group
    group = new QTreeWidgetItem(ui->treeWidget, QStringList(tr("Device")));
    if (mlt->repository()->producers()->get_data("decklink")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("SDI/HDMI")));
        item->setData(0, Qt::UserRole, ui->decklinkTab->objectName());
    }
#ifdef Q_WS_X11
    QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Video4Linux")));
    item->setData(0, Qt::UserRole, ui->v4lTab->objectName());
    saveDefaultPreset("video4linux2");
#endif

    // populate the generators
    group = new QTreeWidgetItem(ui->treeWidget, QStringList(tr("Generator")));
    if (mlt->repository()->producers()->get_data("color")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Color")));
        item->setData(0, Qt::UserRole, ui->colorTab->objectName());
    }
    if (mlt->repository()->producers()->get_data("noise")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Noise")));
        item->setData(0, Qt::UserRole, ui->noiseTab->objectName());
    }
    if (mlt->repository()->producers()->get_data("frei0r.ising0r")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Ising")));
        item->setData(0, Qt::UserRole, ui->isingTab->objectName());
    }
    if (mlt->repository()->producers()->get_data("frei0r.lissajous0r")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Lissajous")));
        item->setData(0, Qt::UserRole, ui->lissajousTab->objectName());
    }
    if (mlt->repository()->producers()->get_data("frei0r.plasma")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Plasma")));
        item->setData(0, Qt::UserRole, ui->plasmaTab->objectName());
    }
    if (mlt->repository()->producers()->get_data("frei0r.test_pat_B")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Color Bars")));
        item->setData(0, Qt::UserRole, ui->colorbarsTab->objectName());
    }
    ui->treeWidget->expandAll();
}

OpenOtherDialog::~OpenOtherDialog()
{
    delete ui;
}

QString OpenOtherDialog::producerName() const
{
    if (ui->methodTabWidget->currentWidget() == ui->networkTab)
        return ui->networkWidget->producerName();
    else if (ui->methodTabWidget->currentWidget() == ui->decklinkTab)
        return ui->decklinkWidget->producerName();
    else if (ui->methodTabWidget->currentWidget() == ui->colorTab)
        return ui->colorWidget->producerName();
    else if (ui->methodTabWidget->currentWidget() == ui->noiseTab)
        return "noise";
    else if (ui->methodTabWidget->currentWidget() == ui->isingTab)
        return ui->isingWidget->producerName();
    else if (ui->methodTabWidget->currentWidget() == ui->lissajousTab)
        return ui->lissajousWidget->producerName();
    else if (ui->methodTabWidget->currentWidget() == ui->plasmaTab)
        return ui->plasmaWidget->producerName();
    else if (ui->methodTabWidget->currentWidget() == ui->v4lTab)
        return ui->v4lWidget->producerName();
    else if (ui->methodTabWidget->currentWidget() == ui->colorbarsTab)
        return ui->colorbarsWidget->producerName();
    else
        return "color";
}

QString OpenOtherDialog::URL(const QString& producer ) const
{
    if (producer == "avformat")
        return ui->networkWidget->URL();
    else if (producer == "decklink")
        return ui->decklinkWidget->URL();
    else if (producer == "video4linux2")
        return ui->v4lWidget->URL();
    else if (producer == "frei0r.plasma")
        return ui->plasmaWidget->URL();
    else
        return producer + ":";
}

QString OpenOtherDialog::URL() const
{
    return URL(producerName());
}

Mlt::Properties* OpenOtherDialog::mltProperties() const
{
    return mltProperties(producerName());
}

Mlt::Properties* OpenOtherDialog::mltProperties(const QString& producer) const
{
    Mlt::Properties* props = 0;
    if (producer == "color")
        props = ui->colorWidget->mltProperties();
    else if (producer == "frei0r.ising0r")
        props = ui->isingWidget->mltProperties();
    else if (producer == "frei0r.lissajous0r")
        props = ui->lissajousWidget->mltProperties();
    else if (producer == "frei0r.plasma")
        props = ui->plasmaWidget->mltProperties();
    else if (producer == "frei0r.test_pat_B")
        props = ui->colorbarsWidget->mltProperties();
    else
        props = new Mlt::Properties;
    return props;
}

void OpenOtherDialog::loadPresets()
{
    // build the presets combo
    ui->presetCombo->clear();
    QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    if (dir.cd("presets") && dir.cd("producer")) {
        ui->presetCombo->addItems(dir.entryList(QDir::Files));
        QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Executable);
        foreach (QString s, entries) {
            if (s == producerName() && dir.cd(s)) {
                ui->presetCombo->addItem("", "");
                QStringList entries2 = dir.entryList(QDir::Files | QDir::Readable);
                foreach (QString s2, entries2) {
                    ui->presetCombo->addItem(s2, s); // userData contains the producer name
                }
                dir.cdUp();
            }
        }
    }
}

void OpenOtherDialog::saveDefaultPreset(const QString& producer)
{
    QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    Mlt::Properties* props = mltProperties(producer);

    if (!dir.exists())
        dir.mkpath(dir.path());
    if (!dir.cd("presets")) {
        if (dir.mkdir("presets"))
            dir.cd("presets");
    }
    if (!dir.cd("producer")) {
        if (dir.mkdir("producer"))
            dir.cd("producer");
    }
    if (!dir.cd(producer)) {
        if (dir.mkdir(producer))
            dir.cd(producer);
    }
    if (!QFile(dir.filePath(tr("<defaults>"))).exists()) {
        props->set("URL", URL(producer).toUtf8().constData());
        props->save(dir.filePath(tr("<defaults>")).toUtf8().constData());
    }
}

void OpenOtherDialog::on_savePresetButton_clicked()
{
    QString preset = QInputDialog::getText(this, tr("Save Preset"), tr("Name:") );
    if (!preset.isNull()) {
        QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
        QString producer = producerName();
        Mlt::Properties* props = mltProperties();

        if (!dir.exists())
            dir.mkpath(dir.path());
        if (!dir.cd("presets")) {
            if (dir.mkdir("presets"))
                dir.cd("presets");
        }
        if (!dir.cd("producer")) {
            if (dir.mkdir("producer"))
                dir.cd("producer");
        }
        if (!dir.cd(producer)) {
            if (dir.mkdir(producer))
                dir.cd(producer);
        }
        if (!props)
            props = new Mlt::Properties;
        props->set("URL", URL().toUtf8().constData());
        props->save(dir.filePath(preset).toUtf8().constData());

        // add the preset and select it
        loadPresets();
        for (int i = 0; i < ui->presetCombo->count(); i++) {
            if (ui->presetCombo->itemText(i) == preset) {
                ui->presetCombo->setCurrentIndex(i);
                break;
            }
        }
    }
}

void OpenOtherDialog::selectTreeWidget(const QString& s)
{
    for (int j = 0; j < ui->treeWidget->topLevelItemCount(); j++) {
        QTreeWidgetItem* group = ui->treeWidget->topLevelItem(j);
        for (int i = 0; i < group->childCount(); i++) {
            if (group->child(i)->text(0) == s) {
                ui->treeWidget->setCurrentItem(group->child(i));
                return;
            }
        }
    }
}

void OpenOtherDialog::load(QString& producer, Mlt::Properties& p)
{
    if (producer == "avformat") {
        selectTreeWidget(tr("Network"));
        ui->networkWidget->load(p);
    }
    else if (producer == "decklink") {
        selectTreeWidget(tr("SDI/HDMI"));
        ui->decklinkWidget->load(p);
    }
    else if (producer == "color") {
        selectTreeWidget(tr("Color"));
        ui->colorWidget->load(p);
    }
    else if (producer == "noise") {
        selectTreeWidget(tr("Noise"));
    }
    else if (producer == "frei0r.ising0r") {
        selectTreeWidget(tr("Ising"));
        ui->isingWidget->load(p);
    }
    else if (producer == "frei0r.lissajous0r") {
        selectTreeWidget(tr("Lissajous"));
        ui->lissajousWidget->load(p);
    }
    else if (producer == "frei0r.plasma") {
        selectTreeWidget(tr("Plasma"));
        ui->plasmaWidget->load(p);
    }
    else if (producer == "video4linux2") {
        selectTreeWidget(tr("Video4Linux"));
        ui->v4lWidget->load(p);
    }
    else if (producer == "frei0r.test_pat_B") {
        selectTreeWidget(tr("Color Bars"));
        ui->colorbarsWidget->load(p);
    }
}

void OpenOtherDialog::on_presetCombo_activated(int index)
{
    QString producer = ui->presetCombo->itemData(index).toString();
    QString preset = ui->presetCombo->itemText(index);
    QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    Mlt::Properties p;
    
    if (!dir.cd("presets") || !dir.cd("producer") || !dir.cd(producer))
        return;
    p.load(dir.filePath(preset).toUtf8().constData());
    load(producer, p);
 }

void OpenOtherDialog::on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (current->data(0, Qt::UserRole).isValid()) {
        for (int i = 0; i < ui->methodTabWidget->count(); i++) {
            if (ui->methodTabWidget->widget(i)->objectName() == current->data(0, Qt::UserRole).toString()) {
                ui->methodTabWidget->setCurrentIndex(i);
                loadPresets();
                break;
            }
        }
    }
}

void OpenOtherDialog::on_deletePresetButton_clicked()
{
    QString preset = ui->presetCombo->currentText();
    int result = QMessageBox::question(this, tr("Delete Preset"),
                                       tr("Are you sure you want to delete") + " " + preset + "?",
                                       QMessageBox::No | QMessageBox::Yes, QMessageBox::No);
    if (result == QMessageBox::Yes) {
        QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
        QString producer = producerName();

        if (dir.cd("presets") && dir.cd("producer") && dir.cd(producer))
            QFile(dir.filePath(preset)).remove();

        ui->presetCombo->removeItem(ui->presetCombo->currentIndex());
        ui->presetCombo->setCurrentIndex(0);
    }
}
