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

#include "servicepresetwidget.h"
#include "ui_servicepresetwidget.h"
#include "qmltypes/qmlapplication.h"
#include "settings.h"

#include <QDir>
#include <QInputDialog>
#include <QMessageBox>

ServicePresetWidget::ServicePresetWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ServicePresetWidget),
    m_widgetName(parent->objectName())
{
    ui->setupUi(this);
}

ServicePresetWidget::~ServicePresetWidget()
{
    delete ui;
}

// This should be called after saveDefaultPreset()
void ServicePresetWidget::loadPresets()
{
    // build the presets combo
    ui->presetCombo->clear();
    QDir dir(Settings.appDataLocation());
    if (dir.cd("presets")) {
        ui->presetCombo->addItems(dir.entryList(QDir::Files));
        QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Executable);
        foreach (QString s, entries) {
            if (s == m_widgetName && dir.cd(s)) {
                ui->presetCombo->addItem("", "");
                QStringList entries2 = dir.entryList(QDir::Files | QDir::Readable);
                foreach (QString s2, entries2)
                    ui->presetCombo->addItem(s2);
                dir.cdUp();
            }
        }
    }
}

void ServicePresetWidget::saveDefaultPreset(Mlt::Properties& properties)
{
    QDir dir(Settings.appDataLocation());

    if (!dir.exists())
        dir.mkpath(dir.path());
    if (!dir.cd("presets")) {
        if (dir.mkdir("presets"))
            dir.cd("presets");
    }
    if (!dir.cd(m_widgetName)) {
        if (dir.mkdir(m_widgetName))
            dir.cd(m_widgetName);
    }
    properties.save(dir.filePath(tr("(defaults)")).toUtf8().constData());
}

void ServicePresetWidget::savePreset(Mlt::Properties* properties)
{
    QInputDialog dialog(this);
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setWindowTitle(tr("Save Preset"));
    dialog.setLabelText(tr("Name:"));
    dialog.setWindowModality(QmlApplication::dialogModality());
    int r = dialog.exec();
    QString preset = dialog.textValue();
    if (r == QDialog::Accepted && !preset.isEmpty()) {
        QDir dir(Settings.appDataLocation());

        if (!dir.exists())
            dir.mkpath(dir.path());
        if (!dir.cd("presets")) {
            if (dir.mkdir("presets"))
                dir.cd("presets");
        }
        if (!dir.cd(m_widgetName)) {
            if (dir.mkdir(m_widgetName))
                dir.cd(m_widgetName);
        }
        properties->save(dir.filePath(preset).toUtf8().constData());

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

void ServicePresetWidget::on_presetCombo_activated(int index)
{
    QString preset = ui->presetCombo->itemText(index);
    QDir dir(Settings.appDataLocation());
    Mlt::Properties* properties = new Mlt::Properties;

    if (!dir.cd("presets") || !dir.cd(m_widgetName))
        return;
    properties->load(dir.filePath(preset).toUtf8().constData());
    emit selected(properties);
}

void ServicePresetWidget::on_savePresetButton_clicked()
{
    emit saveClicked();
}

void ServicePresetWidget::on_deletePresetButton_clicked()
{
    QString preset = ui->presetCombo->currentText();
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
        QDir dir(Settings.appDataLocation());

        if (dir.cd("presets") && dir.cd(m_widgetName))
            QFile(dir.filePath(preset)).remove();

        ui->presetCombo->removeItem(ui->presetCombo->currentIndex());
        ui->presetCombo->setCurrentIndex(0);
    }
}
