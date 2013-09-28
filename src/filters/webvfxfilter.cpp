/*
 * Copyright (c) 2013 Meltytech, LLC
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

#include <QSettings>
#include <QStandardPaths>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "webvfxfilter.h"
#include "ui_webvfxfilter.h"
#include "mltcontroller.h"
#include "mainwindow.h"
#include "htmleditor/htmleditor.h"

WebvfxFilter::WebvfxFilter(Mlt::Filter filter, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WebvfxFilter),
    m_filter(filter)
{
    ui->setupUi(this);
    QString resource = filter.get("resource");
    if (resource.startsWith("plain:"))
        resource.remove(0, 6);
    ui->fileLabel->setText(QFileInfo(resource).fileName());
    ui->fileLabel->setToolTip(resource);
    if (resource.isEmpty()) {
        ui->editButton->setDisabled(true);
        ui->reloadButton->setDisabled(true);
    } else {
        ui->newButton->setDisabled(true);
        ui->openButton->setDisabled(true);
    }
    m_filter.set_in_and_out(0, MLT.producer()->get_playtime() - 1);
}

WebvfxFilter::~WebvfxFilter()
{
    delete ui;
}

void WebvfxFilter::on_openButton_clicked()
{
    QSettings settings;
    QString settingKey("openPath");
    QString directory(settings.value(settingKey,
        QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString());
    QString filename = QFileDialog::getOpenFileName(this, tr("Open HTML File"), directory,
        tr("HTML-Files (*.htm *.html);;All Files (*)"));
    activateWindow();
    if (!filename.isEmpty())
        setFilterFileName(filename);
}

void WebvfxFilter::on_editButton_clicked()
{
    ui->editButton->setDisabled(true);
    MAIN.editHTML(ui->fileLabel->toolTip());
    connect(MAIN.htmlEditor(), SIGNAL(closed()), SLOT(onHtmlClosed()));
    connect(MAIN.htmlEditor(), SIGNAL(saved()), SLOT(on_reloadButton_clicked()));
}

void WebvfxFilter::onHtmlClosed()
{
    ui->editButton->setEnabled(true);
}

void WebvfxFilter::on_reloadButton_clicked()
{
    m_filter.set_in_and_out(0, MLT.producer()->get_playtime() - 1);
    m_filter.set("_reload", 1);
    MLT.refreshConsumer();
}

void WebvfxFilter::on_webvfxCheckBox_clicked(bool checked)
{
    if (!checked) return;
    QMessageBox dialog(QMessageBox::Question,
                       tr("Comfirm Selection"),
                       ui->webvfxCheckBox->toolTip() + "\n" +
                       tr("Do you still want to use this?"),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(Qt::WindowModal);
    if (dialog.exec() == QMessageBox::No)
        ui->webvfxCheckBox->setChecked(false);
}

void WebvfxFilter::on_newButton_clicked()
{
    QSettings settings;
    QString settingKey("openPath");
    QString directory(settings.value(settingKey,
        QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString());
    QString filename = QFileDialog::getSaveFileName(this, tr("Open HTML File"), directory,
        tr("HTML-Files (*.htm *.html);;All Files (*)"));
    activateWindow();

    if (!filename.isEmpty()) {
        QFile file(filename);
        file.open(QFile::WriteOnly);
        file.write("<html></html>");
        file.close();
        setFilterFileName(filename);
    }
}

void WebvfxFilter::setFilterFileName(QString filename)
{
    QFileInfo info(filename);
    QSettings settings;
    QString settingKey("openPath");
    settings.setValue(settingKey, info.path());
    ui->newButton->setDisabled(true);
    ui->openButton->setDisabled(true);
    ui->fileLabel->setText(info.fileName());
    ui->fileLabel->setToolTip(filename);
    if (!ui->webvfxCheckBox->isChecked())
        filename.prepend("plain:");
    m_filter.set("resource", filename.toUtf8().constData());
    MLT.refreshConsumer();
    if (info.suffix().startsWith("htm"))
        ui->editButton->setEnabled(true);
    ui->reloadButton->setEnabled(true);
    ui->webvfxCheckBox->setDisabled(true);
}
