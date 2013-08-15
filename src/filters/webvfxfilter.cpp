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

#include "webvfxfilter.h"
#include "ui_webvfxfilter.h"
#include "mltcontroller.h"

WebvfxFilter::WebvfxFilter(Mlt::Filter filter, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WebvfxFilter),
    m_filter(filter)
{
    ui->setupUi(this);
    ui->fileLabel->setText(QFileInfo(filter.get("resource")).fileName());
    ui->fileLabel->setToolTip(filter.get("resource"));
}

WebvfxFilter::~WebvfxFilter()
{
    delete ui;
}

void WebvfxFilter::on_fileButton_clicked()
{
    QSettings settings;
    QString settingKey("openPath");
    QString directory(settings.value(settingKey,
        QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString());
    QString filename = QFileDialog::getOpenFileName(this, tr("Open HTML File"), directory);

    if (!filename.isEmpty()) {
        QFileInfo info(filename);
        settings.setValue(settingKey, info.path());
        activateWindow();
        ui->fileLabel->setText(info.fileName());
        ui->fileLabel->setToolTip(filename);
        m_filter.set("resource", filename.prepend("plain:").toUtf8().constData());
        MLT.refreshConsumer();
    }
}

void WebvfxFilter::on_editButton_clicked()
{
    
}
