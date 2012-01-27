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

enum {
    NetworkTabIndex = 0,
    DeckLinkTabIndex,
    ColorTabIndex,
    NoiseTabIndex,
    IsingTabIndex,
    LissajousTabIndex,
    PlasmaTabIndex
};

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
        item->setData(0, Qt::UserRole, 1);
    }
#ifdef Q_WS_X11
    QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Video4Linux")));
    item->setData(0, Qt::UserRole, 7);
    saveDefaultPreset("video4linux2");
#endif

    // populate the generators
    group = new QTreeWidgetItem(ui->treeWidget, QStringList(tr("Generator")));
    if (mlt->repository()->producers()->get_data("color")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Color")));
        item->setData(0, Qt::UserRole, 2);
    }
    if (mlt->repository()->producers()->get_data("noise")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Noise")));
        item->setData(0, Qt::UserRole, 3);
    }
    if (mlt->repository()->producers()->get_data("frei0r.ising0r")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Ising")));
        item->setData(0, Qt::UserRole, 4);
    }
    if (mlt->repository()->producers()->get_data("frei0r.lissajous0r")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Lissajous")));
        item->setData(0, Qt::UserRole, 5);
    }
    if (mlt->repository()->producers()->get_data("frei0r.plasma")) {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList(tr("Plasma")));
        item->setData(0, Qt::UserRole, 6);
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
        return "avformat";
    else if (ui->methodTabWidget->currentWidget() == ui->decklinkTab)
        return "decklink";
    else if (ui->methodTabWidget->currentWidget() == ui->colorTab)
        return "color";
    else if (ui->methodTabWidget->currentWidget() == ui->noiseTab)
        return "noise";
    else if (ui->methodTabWidget->currentWidget() == ui->isingTab)
        return "frei0r.ising0r";
    else if (ui->methodTabWidget->currentWidget() == ui->lissajousTab)
        return "frei0r.lissajous0r";
    else if (ui->methodTabWidget->currentWidget() == ui->plasmaTab)
        return "frei0r.plasma";
    else if (ui->methodTabWidget->currentWidget() == ui->v4lTab)
        return "video4linux2";
    else
        return "color";
}

QString OpenOtherDialog::URL(const QString& producer ) const
{
    if (producer == "avformat")
        return ui->urlLineEdit->text();
    else if (producer == "decklink")
        return QString("decklink:%1").arg(ui->decklinkCardSpinner->value());
    else if (producer == "video4linux2") {
        QString s = QString("video4linux2:%1?width=%2&height=%3&framerate=%4")
                .arg(ui->v4lLineEdit->text())
                .arg(ui->v4lWidthSpinBox->value())
                .arg(ui->v4lHeightSpinBox->value())
                .arg(ui->v4lFramerateSpinBox->value());
        if (ui->v4lStandardCombo->currentIndex() > 0)
            s += QString("&standard=") + ui->v4lStandardCombo->currentText();
        if (ui->v4lChannelSpinBox->value() > 0)
            s += QString("&channel=%1").arg(ui->v4lChannelSpinBox->value());
        return s;
    }
    else
        return producer + ":";
}

QString OpenOtherDialog::URL() const
{
    return URL(producerName());
}

Mlt::Properties* OpenOtherDialog::mltProperties() const
{
    Mlt::Properties* props = 0;
    if (ui->methodTabWidget->currentWidget() == ui->colorTab) {
        props = new Mlt::Properties;
        props->set("colour", ui->colorLabel->text().toAscii().constData());
    }
    else if (ui->methodTabWidget->currentWidget() == ui->isingTab) {
        props = new Mlt::Properties;
        props->set("Temperature", ui->tempSpinner->text().toAscii().constData());
        props->set("Border Growth", ui->borderGrowthSpinner->text().toAscii().constData());
        props->set("Spontaneous Growth", ui->spontGrowthSpinner->text().toAscii().constData());
    }
    else if (ui->methodTabWidget->currentWidget() == ui->lissajousTab) {
        props = new Mlt::Properties;
        props->set("ratiox", ui->xratioSpinner->text().toAscii().constData());
        props->set("ratioy", ui->yratioSpinner->text().toAscii().constData());
    }
    else if (ui->methodTabWidget->currentWidget() == ui->plasmaTab) {
        props = new Mlt::Properties;
        props->set("1_speed", ui->speed1Spinner->text().toAscii().constData());
        props->set("2_speed", ui->speed2Spinner->text().toAscii().constData());
        props->set("3_speed", ui->speed3Spinner->text().toAscii().constData());
        props->set("4_speed", ui->speed4Spinner->text().toAscii().constData());
        props->set("1_move", ui->move1Spinner->text().toAscii().constData());
        props->set("2_move", ui->move2Spinner->text().toAscii().constData());
    }
    return props;
}

Mlt::Properties* OpenOtherDialog::mltProperties(const QString& producer) const
{
    Mlt::Properties* props = new Mlt::Properties;
    if (producer == "color") {
        props->set("colour", ui->colorLabel->text().toAscii().constData());
    }
    else if (producer == "frei0r.ising0r") {
        props->set("Temperature", ui->tempSpinner->text().toAscii().constData());
        props->set("Border Growth", ui->borderGrowthSpinner->text().toAscii().constData());
        props->set("Spontaneous Growth", ui->spontGrowthSpinner->text().toAscii().constData());
    }
    else if (producer == "frei0r.ising0r") {
        props->set("ratiox", ui->xratioSpinner->text().toAscii().constData());
        props->set("ratioy", ui->yratioSpinner->text().toAscii().constData());
    }
    else if (producer == "frei0r.plasma") {
        props->set("1_speed", ui->speed1Spinner->text().toAscii().constData());
        props->set("2_speed", ui->speed2Spinner->text().toAscii().constData());
        props->set("3_speed", ui->speed3Spinner->text().toAscii().constData());
        props->set("4_speed", ui->speed4Spinner->text().toAscii().constData());
        props->set("1_move", ui->move1Spinner->text().toAscii().constData());
        props->set("2_move", ui->move2Spinner->text().toAscii().constData());
    }
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

void OpenOtherDialog::on_colorButton_clicked()
{
    QColorDialog dialog;
    dialog.setOption(QColorDialog::ShowAlphaChannel);
    if (dialog.exec() == QDialog::Accepted) {
        ui->colorLabel->setText(QString().sprintf("#%02X%02X%02X%02X",
                                                  qAlpha(dialog.currentColor().rgba()),
                                                  qRed(dialog.currentColor().rgba()),
                                                  qGreen(dialog.currentColor().rgba()),
                                                  qBlue(dialog.currentColor().rgba())
                                                  ));
        ui->colorLabel->setStyleSheet(QString("background-color: %1").arg(dialog.currentColor().name()));
    }
}

///////////////// Ising //////////////////////

void OpenOtherDialog::on_tempDial_valueChanged(int value)
{
    ui->tempSpinner->setValue(value/100.0);
}

void OpenOtherDialog::on_tempSpinner_valueChanged(double value)
{
    ui->tempDial->setValue(value * 100);
}

void OpenOtherDialog::on_borderGrowthDial_valueChanged(int value)
{
    ui->borderGrowthSpinner->setValue(value/100.0);    
}

void OpenOtherDialog::on_borderGrowthSpinner_valueChanged(double value)
{
    ui->borderGrowthDial->setValue(value * 100);
}

void OpenOtherDialog::on_spontGrowthDial_valueChanged(int value)
{
    ui->spontGrowthSpinner->setValue(value/100.0);
}

void OpenOtherDialog::on_spontGrowthSpinner_valueChanged(double value)
{
    ui->spontGrowthDial->setValue(value * 100);
}

/////////////////// Lissajous ///////////////////////////////

void OpenOtherDialog::on_xratioDial_valueChanged(int value)
{
    ui->xratioSpinner->setValue(value/100.0);
}

void OpenOtherDialog::on_xratioSpinner_valueChanged(double value)
{
    ui->xratioDial->setValue(value * 100);
}

void OpenOtherDialog::on_yratioDial_valueChanged(int value)
{
    ui->yratioSpinner->setValue(value/100.0);    
}

void OpenOtherDialog::on_yratioSpinner_valueChanged(double value)
{
    ui->yratioDial->setValue(value * 100);
}

///////////////////// Plasma /////////////////////

void OpenOtherDialog::on_speed1Dial_valueChanged(int value)
{
    ui->speed1Spinner->setValue(value/100.0);
}

void OpenOtherDialog::on_speed1Spinner_valueChanged(double value)
{
    ui->speed1Dial->setValue(value * 100);
}

void OpenOtherDialog::on_speed2Dial_valueChanged(int value)
{
    ui->speed2Spinner->setValue(value/100.0);    
}

void OpenOtherDialog::on_speed2Spinner_valueChanged(double value)
{
    ui->speed2Dial->setValue(value * 100);
}

void OpenOtherDialog::on_speed3Dial_valueChanged(int value)
{
    ui->speed3Spinner->setValue(value/100.0);
}

void OpenOtherDialog::on_speed3Spinner_valueChanged(double value)
{
    ui->speed3Dial->setValue(value * 100);
}

void OpenOtherDialog::on_speed4Dial_valueChanged(int value)
{
    ui->speed4Spinner->setValue(value/100.0);
}

void OpenOtherDialog::on_speed4Spinner_valueChanged(double value)
{
    ui->speed4Dial->setValue(value * 100);
}

void OpenOtherDialog::on_move1Dial_valueChanged(int value)
{
    ui->move1Spinner->setValue(value/100.0);    
}

void OpenOtherDialog::on_move1Spinner_valueChanged(double value)
{
    ui->move1Dial->setValue(value * 100);
}

void OpenOtherDialog::on_move2Dial_valueChanged(int value)
{
    ui->move2Spinner->setValue(value/100.0);    
}

void OpenOtherDialog::on_move2Spinner_valueChanged(double value)
{
    ui->move2Dial->setValue(value * 100);
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
        ui->urlLineEdit->setText(p.get("URL"));
    }
    else if (producer == "decklink") {
        selectTreeWidget(tr("SDI/HDMI"));
        QString s(p.get("URL"));
        ui->decklinkCardSpinner->setValue(s.mid(s.indexOf(':') + 1).toInt());
    }
    else if (producer == "color") {
        selectTreeWidget(tr("Color"));
        ui->colorLabel->setText(p.get("colour"));
        ui->colorLabel->setStyleSheet(QString("background-color: %1")
            .arg(QString(p.get("colour")).replace(0, 3, "#")));
    }
    else if (producer == "noise") {
        selectTreeWidget(tr("Noise"));
    }
    else if (producer == "frei0r.ising0r") {
        selectTreeWidget(tr("Ising"));
        ui->tempSpinner->setValue(p.get_double("Temperature"));
        ui->borderGrowthSpinner->setValue(p.get_double("Border Growth"));
        ui->spontGrowthSpinner->setValue(p.get_double("Spontaneous Growth"));
    }
    else if (producer == "frei0r.lissajous0r") {
        selectTreeWidget(tr("Lissajous"));
        ui->xratioSpinner->setValue(p.get_double("ratiox"));
        ui->yratioSpinner->setValue(p.get_double("ratioy"));
    }
    else if (producer == "frei0r.plasma") {
        selectTreeWidget(tr("Plasma"));
        ui->speed1Spinner->setValue(p.get_double("1_speed"));
        ui->speed2Spinner->setValue(p.get_double("2_speed"));
        ui->speed3Spinner->setValue(p.get_double("3_speed"));
        ui->speed4Spinner->setValue(p.get_double("4_speed"));
        ui->move1Spinner->setValue(p.get_double("1_move"));
        ui->move2Spinner->setValue(p.get_double("1_move"));
    }
    else if (producer == "video4linux2") {
        selectTreeWidget(tr("Video4Linux"));
        QString s(p.get("URL"));
        s = s.mid(s.indexOf(':') + 1); // chomp video4linux2:
        QString device = s.mid(0, s.indexOf('?'));
        ui->v4lLineEdit->setText(device);
        s = s.mid(s.indexOf('?') + 1); // chomp device
        if (s.indexOf('&') != -1)
        foreach (QString item, s.split('&')) {
            if (item.indexOf('=') != -1) {
                QStringList pair = item.split('=');
                if (pair.at(0) == "width")
                    ui->v4lWidthSpinBox->setValue(pair.at(1).toInt());
                else if (pair.at(0) == "height")
                    ui->v4lHeightSpinBox->setValue(pair.at(1).toInt());
                else if (pair.at(0) == "framerate")
                    ui->v4lFramerateSpinBox->setValue(pair.at(1).toDouble());
                else if (pair.at(0) == "channel")
                    ui->v4lChannelSpinBox->setValue(pair.at(1).toInt());
                else if (pair.at(0) == "standard") {
                    for (int i = 0; i < ui->v4lStandardCombo->count(); i++) {
                        if (ui->v4lStandardCombo->itemText(i) == pair.at(1)) {
                            ui->v4lStandardCombo->setCurrentIndex(i);
                            break;
                        }
                    }
                }
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
    load(producer, p);
 }

void OpenOtherDialog::on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (current->data(0, Qt::UserRole).isValid()) {
        ui->methodTabWidget->setCurrentIndex(current->data(0, Qt::UserRole).toInt());
        loadPresets();
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
