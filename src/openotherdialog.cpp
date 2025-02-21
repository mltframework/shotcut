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
    m_addTimelineButton = ui->buttonBox->addButton(tr("Add To Timeline"),
                                                   QDialogButtonBox::ApplyRole);
    connect(m_addTimelineButton, &QPushButton::clicked, this, [=]() { done(-1); });

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

    // populate the generators
    group = new QTreeWidgetItem(ui->treeWidget, QStringList(tr("Generator")));
    if (mltProducers->get_data("color")) {
        QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Color")));
        item->setData(0, Qt::UserRole, ui->colorTab->objectName());
        if (mltProducers->get_data("qtext") && mltFilters->get_data("dynamictext")) {
            QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Text")));
            item->setData(0, Qt::UserRole, ui->textTab->objectName());
        }
    }
    if (mltProducers->get_data("glaxnimate")) {
        QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Animation")));
        item->setData(0, Qt::UserRole, ui->glaxnimateTab->objectName());
    }
    if (mltProducers->get_data("noise")) {
        QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Noise")));
        item->setData(0, Qt::UserRole, ui->noiseTab->objectName());
    }
    if (mltProducers->get_data("frei0r.ising0r")) {
        QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Ising")));
        item->setData(0, Qt::UserRole, ui->isingTab->objectName());
    }
    if (mltProducers->get_data("frei0r.lissajous0r")) {
        QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Lissajous")));
        item->setData(0, Qt::UserRole, ui->lissajousTab->objectName());
    }
    if (mltProducers->get_data("frei0r.plasma")) {
        QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Plasma")));
        item->setData(0, Qt::UserRole, ui->plasmaTab->objectName());
    }
    if (mltProducers->get_data("frei0r.test_pat_B")) {
        QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Color Bars")));
        item->setData(0, Qt::UserRole, ui->colorbarsTab->objectName());
    }
    if (mltProducers->get_data("tone")) {
        QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Audio Tone")));
        item->setData(0, Qt::UserRole, ui->toneTab->objectName());
    }
    if (mltProducers->get_data("count")) {
        QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Count")));
        item->setData(0, Qt::UserRole, ui->countTab->objectName());
    }
    if (mltProducers->get_data("blipflash")) {
        QTreeWidgetItem *item = new QTreeWidgetItem(group, QStringList(tr("Blip Flash")));
        item->setData(0, Qt::UserRole, ui->blipTab->objectName());
    }
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
        else if (service == "color")
            selectTreeWidget(tr("Color"));
        else if (service == "glaxnimate")
            selectTreeWidget(tr("Animation"));
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
        else if (service == "tone")
            selectTreeWidget(tr("Audio Tone"));
        else if (service == "count")
            selectTreeWidget(tr("Count"));
        else if (service == "blipflash")
            selectTreeWidget(tr("Blip Flash"));
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
        m_addTimelineButton->setVisible(true);
        for (int i = 0; i < ui->methodTabWidget->count(); i++) {
            QString tabName(ui->methodTabWidget->widget(i)->objectName());
            if (currentData == tabName) {
                ui->methodTabWidget->setCurrentIndex(i);
                QWidget *w = ui->methodTabWidget->currentWidget();
                if (w == ui->networkTab) {
                    m_current = ui->networkWidget;
                    m_addTimelineButton->setVisible(false);
                } else if (w == ui->decklinkTab) {
                    m_current = ui->decklinkWidget;
                    m_addTimelineButton->setVisible(false);
                } else if (w == ui->v4lTab) {
                    m_current = ui->v4lWidget;
                    m_addTimelineButton->setVisible(false);
                } else if (w == ui->colorTab) {
                    m_current = ui->colorWidget;
                } else if (w == ui->textTab) {
                    m_current = ui->textWidget;
                } else if (w == ui->glaxnimateTab) {
                    m_current = ui->glaxnimateWidget;
                } else if (w == ui->noiseTab) {
                    m_current = ui->noiseWidget;
                } else if (w == ui->isingTab) {
                    m_current = ui->isingWidget;
                } else if (w == ui->lissajousTab) {
                    m_current = ui->lissajousWidget;
                } else if (w == ui->plasmaTab) {
                    m_current = ui->plasmaWidget;
                } else if (w == ui->colorbarsTab) {
                    m_current = ui->colorbarsWidget;
                } else if (w == ui->pulseTab) {
                    m_current = ui->pulseWidget;
                    m_addTimelineButton->setVisible(false);
                } else if (w == ui->alsaTab) {
                    m_current = ui->alsaWidget;
                    m_addTimelineButton->setVisible(false);
                } else if (w == ui->dshowVideoTab) {
                    m_current = ui->dshowVideoWidget;
                    m_addTimelineButton->setVisible(false);
                } else if (w == ui->toneTab) {
                    m_current = ui->toneWidget;
                } else if (w == ui->countTab) {
                    m_current = ui->countWidget;
                } else if (w == ui->avfoundationTab) {
                    m_current = ui->avfoundationWidget;
                    m_addTimelineButton->setVisible(false);
                } else if (w == ui->blipTab) {
                    m_current = ui->blipWidget;
                }
                break;
            }
        }
    }
}
