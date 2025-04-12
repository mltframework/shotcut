/*
 * Copyright (c) 2012-2025 Meltytech, LLC
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
#include <QtWidgets>

OpenOtherDialog::OpenOtherDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OpenOtherDialog)
{
    ui->setupUi(this);
    m_current = ui->networkWidget;

    QScopedPointer<Mlt::Properties> mltProducers(MLT.repository()->producers());
    QScopedPointer<Mlt::Properties> mltFilters(MLT.repository()->filters());
    QTreeWidgetItem *group = new QTreeWidgetItem(ui->treeWidget, QStringList(tr("Network")));
    group->setData(0, Qt::UserRole, ui->networkTab->objectName());
    ui->treeWidget->setCurrentItem(group);

    // populate the device group
    group = new QTreeWidgetItem(ui->treeWidget, QStringList(tr("Device")));
    if (mltProducers->get_data("decklink")) {
        QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("SDI/HDMI")));
        item->setData(0, Qt::UserRole, ui->decklinkTab->objectName());
    }
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Video4Linux")));
    item->setData(0, Qt::UserRole, ui->v4lTab->objectName());
    item = new QTreeWidgetItem(group, QStringList(tr("PulseAudio")));
    item->setData(0, Qt::UserRole, ui->pulseTab->objectName());
    item = new QTreeWidgetItem(group, QStringList(tr("ALSA Audio")));
    item->setData(0, Qt::UserRole, ui->alsaTab->objectName());
#elif defined(Q_OS_WIN)
    QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Audio/Video Device")));
    item->setData(0, Qt::UserRole, ui->dshowVideoTab->objectName());
#elif defined(Q_OS_MAC)
    QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Audio/Video Device")));
    item->setData(0, Qt::UserRole, ui->avfoundationTab->objectName());
#endif

    ui->treeWidget->expandAll();
}

OpenOtherDialog::~OpenOtherDialog()
{
    delete ui;
}

Mlt::Producer *OpenOtherDialog::newProducer(Mlt::Profile &profile, QObject *widget) const
{
    return dynamic_cast<AbstractProducerWidget *>(widget)->newProducer(profile);
}

Mlt::Producer *OpenOtherDialog::newProducer(Mlt::Profile &profile) const
{
    return newProducer(profile, m_current);
}

void OpenOtherDialog::load(Mlt::Producer *producer)
{
    if (producer && producer->is_valid()) {
        QString service(producer->get("mlt_service"));
        QString resource(MLT.resource());

        if (resource.startsWith("video4linux2:"))
            selectTreeWidget(tr("Video4Linux"));
        else if (resource.startsWith("pulse:"))
            selectTreeWidget(tr("PulseAudio"));
        else if (resource.startsWith("alsa:"))
            selectTreeWidget(tr("ALSA Audio"));
        else if (resource.startsWith("dshow:"))
            selectTreeWidget(tr("Audio/Video Device"));
        else if (service.startsWith("avformat"))
            selectTreeWidget(tr("Network"));
        else if (service == "decklink" || resource.contains("decklink"))
            selectTreeWidget(tr("SDI/HDMI"));
        dynamic_cast<AbstractProducerWidget *>(m_current)->loadPreset(*producer);
    }
}

void OpenOtherDialog::selectTreeWidget(const QString &s)
{
    for (int j = 0; j < ui->treeWidget->topLevelItemCount(); j++) {
        QTreeWidgetItem *group = ui->treeWidget->topLevelItem(j);
        for (int i = 0; i < group->childCount(); i++) {
            if (group->child(i)->text(0) == s) {
                ui->treeWidget->setCurrentItem(group->child(i));
                return;
            }
        }
    }
}

void OpenOtherDialog::on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
    if (current->data(0, Qt::UserRole).isValid()) {
        QString currentData(current->data(0, Qt::UserRole).toString());
        for (int i = 0; i < ui->methodTabWidget->count(); i++) {
            QString tabName(ui->methodTabWidget->widget(i)->objectName());
            if (currentData == tabName) {
                ui->methodTabWidget->setCurrentIndex(i);
                QWidget *w = ui->methodTabWidget->currentWidget();
                if (w == ui->networkTab) {
                    m_current = ui->networkWidget;
                } else if (w == ui->decklinkTab) {
                    m_current = ui->decklinkWidget;
                } else if (w == ui->v4lTab) {
                    m_current = ui->v4lWidget;
                } else if (w == ui->pulseTab) {
                    m_current = ui->pulseWidget;
                } else if (w == ui->alsaTab) {
                    m_current = ui->alsaWidget;
                } else if (w == ui->dshowVideoTab) {
                    m_current = ui->dshowVideoWidget;
                } else if (w == ui->avfoundationTab) {
                    m_current = ui->avfoundationWidget;
                }
                break;
            }
        }
    }
}
