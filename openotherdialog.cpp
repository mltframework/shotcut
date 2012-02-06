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
    m_current = ui->networkWidget;

    saveDefaultPreset(ui->colorWidget);
    saveDefaultPreset(ui->isingWidget);
    saveDefaultPreset(ui->lissajousWidget);
    saveDefaultPreset(ui->plasmaWidget);
    loadPresets();

    QTreeWidgetItem* group = new QTreeWidgetItem(ui->treeWidget, QStringList(tr("Network")));
    group->setData(0, Qt::UserRole, ui->networkTab->objectName());
    ui->treeWidget->setCurrentItem(group);

    // populate the device group
    group = new QTreeWidgetItem(ui->treeWidget, QStringList(tr("Device")));
    if (mlt->repository()->producers()->get_data("decklink")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("SDI/HDMI")));
        item->setData(0, Qt::UserRole, ui->decklinkTab->objectName());
    }
#ifdef Q_WS_X11
    QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Video4Linux")));
    item->setData(0, Qt::UserRole, ui->v4lTab->objectName());
    saveDefaultPreset(ui->v4lWidget);
    item = new QTreeWidgetItem(group, QStringList(tr("PulseAudio")));
    item->setData(0, Qt::UserRole, ui->pulseTab->objectName());
    item = new QTreeWidgetItem(group, QStringList(tr("JACK Audio")));
    item->setData(0, Qt::UserRole, ui->jackTab->objectName());
    item = new QTreeWidgetItem(group, QStringList(tr("ALSA Audio")));
    item->setData(0, Qt::UserRole, ui->alsaTab->objectName());
    item = new QTreeWidgetItem(group, QStringList(tr("Screen")));
    item->setData(0, Qt::UserRole, ui->x11grabTab->objectName());
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

Mlt::Producer* OpenOtherDialog::producer(Mlt::Profile& profile, QObject* widget) const
{
    return dynamic_cast<AbstractProducerWidget*>(widget)->producer(profile);
}

Mlt::Producer* OpenOtherDialog::producer(Mlt::Profile& profile) const
{
    return producer(profile, m_current);
}

void OpenOtherDialog::load(Mlt::Producer* producer)
{
    if (producer && producer->is_valid()) {
        QString service(producer->get("mlt_service"));
        QString resource(producer->get("resource"));
        if (resource.startsWith("video4linux2:"))
            selectTreeWidget(tr("Video4Linux"));
        else if (resource.startsWith("pulse:"))
            selectTreeWidget(tr("PulseAudio"));
        else if (resource.startsWith("jack:"))
            selectTreeWidget(tr("JACK Audio"));
        else if (resource.startsWith("alsa:"))
            selectTreeWidget(tr("ALSA Audio"));
        else if (resource.startsWith("x11grab:"))
            selectTreeWidget(tr("Screen"));
        else if (service == "avformat")
            selectTreeWidget(tr("Network"));
        else if (service == "decklink")
            selectTreeWidget(tr("SDI/HDMI"));
        else if (service == "color")
            selectTreeWidget(tr("Color"));
        else if (service == "noise")
            selectTreeWidget(tr("Noise"));
        else if (service == "frei0r.ising0r")
            selectTreeWidget(tr("Ising"));
        else if (service == "frei0r.lissajous0r")
            selectTreeWidget(tr("Lissajous"));
        else if (service == "frei0r.plasma")
            selectTreeWidget(tr("Plasma"));
        else if (service == "frei0r.test_pat_B")
            selectTreeWidget(tr("Color Bars"));
        dynamic_cast<AbstractProducerWidget*>(m_current)->loadPreset(*producer);
    }
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
            if (s == m_current->objectName() && dir.cd(s)) {
                ui->presetCombo->addItem("", "");
                QStringList entries2 = dir.entryList(QDir::Files | QDir::Readable);
                foreach (QString s2, entries2) {
                    ui->presetCombo->addItem(s2, s); // userData contains the tab name
                }
                dir.cdUp();
            }
        }
    }
}

void OpenOtherDialog::saveDefaultPreset(QObject* widget)
{
    QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    Mlt::Properties* props = dynamic_cast<AbstractProducerWidget*>(widget)->getPreset();
    QString producer(widget->objectName());

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
        props->save(dir.filePath(tr("<defaults>")).toUtf8().constData());
    }
    delete props;
}

void OpenOtherDialog::on_savePresetButton_clicked()
{
    QString preset = QInputDialog::getText(this, tr("Save Preset"), tr("Name:") );
    if (!preset.isNull()) {
        QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
        Mlt::Properties* props = dynamic_cast<AbstractProducerWidget*>(m_current)->getPreset();
        QString producer = m_current->objectName();

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
        props->save(dir.filePath(preset).toUtf8().constData());
        delete props;

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

void OpenOtherDialog::on_presetCombo_activated(int index)
{
    QString producer = ui->presetCombo->itemData(index).toString();
    QString preset = ui->presetCombo->itemText(index);
    QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    Mlt::Properties p;
    
    if (!dir.cd("presets") || !dir.cd("producer") || !dir.cd(producer))
        return;
    p.load(dir.filePath(preset).toUtf8().constData());
    dynamic_cast<AbstractProducerWidget*>(m_current)->loadPreset(p);
 }

void OpenOtherDialog::on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (current->data(0, Qt::UserRole).isValid()) {
        QString currentData(current->data(0, Qt::UserRole).toString());
        for (int i = 0; i < ui->methodTabWidget->count(); i++) {
            QString tabName(ui->methodTabWidget->widget(i)->objectName());
            if (currentData == tabName) {
                ui->methodTabWidget->setCurrentIndex(i);
                QWidget* w = ui->methodTabWidget->currentWidget();
                if (w == ui->networkTab)
                    m_current = ui->networkWidget;
                else if (w == ui->decklinkTab)
                    m_current = ui->decklinkWidget;
                else if (w == ui->v4lTab)
                    m_current = ui->v4lWidget;
                else if (w == ui->colorTab)
                    m_current = ui->colorWidget;
                else if (w == ui->noiseTab)
                    m_current = ui->noiseWidget;
                else if (w == ui->isingTab)
                    m_current = ui->isingWidget;
                else if (w == ui->lissajousTab)
                    m_current = ui->lissajousWidget;
                else if (w == ui->plasmaTab)
                    m_current = ui->plasmaWidget;
                else if (w == ui->colorbarsTab)
                    m_current = ui->colorbarsWidget;
                else if (w == ui->pulseTab)
                    m_current = ui->pulseWidget;
                else if (w == ui->jackTab)
                    m_current = ui->jackWidget;
                else if (w == ui->alsaTab)
                    m_current = ui->alsaWidget;
                else if (w == ui->x11grabTab)
                    m_current= ui->x11grabWidget;
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
        QString producer = m_current->objectName();

        if (dir.cd("presets") && dir.cd("producer") && dir.cd(producer))
            QFile(dir.filePath(preset)).remove();

        ui->presetCombo->removeItem(ui->presetCombo->currentIndex());
        ui->presetCombo->setCurrentIndex(0);
    }
}
