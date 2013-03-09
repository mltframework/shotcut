/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * GL shader based on BSD licensed code from Peter Bengtsson:
 * http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
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

#include "filtersdock.h"
#include "ui_filtersdock.h"
#include "mltcontroller.h"
#include <QtGui/QMenu>
#include <QtCore/QSettings>
#include "mainwindow.h"
#include "filters/movitblurfilter.h"
#include "filters/movitglowfilter.h"

FiltersDock::FiltersDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::FiltersDock)
{
    QSettings settings;
    m_isGPU = settings.value("player/gpu", false).toBool();
    ui->setupUi(this);
    toggleViewAction()->setIcon(QIcon::fromTheme("view-filter", windowIcon()));
    ui->listView->setModel(&m_model);
    connect(model(), SIGNAL(changed()), this, SLOT(onModelChanged()));
}

FiltersDock::~FiltersDock()
{
    delete ui;
}

void FiltersDock::onModelChanged()
{
    MLT.refreshConsumer();
    ui->removeButton->setEnabled(m_model.rowCount() > 0);
}

void FiltersDock::onProducerOpened()
{
    m_model.reset();
    onModelChanged();
    if (MLT.isPlaylist()) {
        ui->addButton->setDisabled(true);
        MAIN.showStatusMessage(tr("Filters can only be applied to clips."));
    }
    else {
        ui->addButton->setEnabled(true);
    }
}

void FiltersDock::on_addButton_clicked()
{
    QPoint pos = ui->addButton->mapToParent(QPoint(0, 0));
    QMenu menu(this);
    menu.addAction(ui->actionBlur);
    if (m_isGPU) menu.addAction(ui->actionDiffusion);
    menu.addAction(ui->actionGlow);
    menu.addAction(ui->actionMirror);
    menu.addAction(ui->actionSharpen);
    menu.addAction(ui->actionVignette);
    menu.exec(mapToGlobal(pos));
}

void FiltersDock::on_removeButton_clicked()
{
    QModelIndex index = ui->listView->currentIndex();
    if (index.isValid()) {
        m_model.remove(index.row());
        delete ui->scrollArea->widget();
    }
}

void FiltersDock::on_actionBlur_triggered()
{
    Mlt::Filter* filter = m_model.add(m_isGPU? "movit.blur" : "boxblur");
    if (filter && filter->is_valid()) {
        if (m_isGPU)
            ui->scrollArea->setWidget(new MovitBlurFilter(*filter, true));
    }
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionMirror_triggered()
{
    m_model.add(m_isGPU? "movit.mirror": "mirror:flip");
    delete ui->scrollArea->widget();
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_listView_doubleClicked(const QModelIndex &index)
{
    m_model.setData(index, true, Qt::CheckStateRole);
}

void FiltersDock::on_actionDiffusion_triggered()
{
    m_model.add("movit.diffusion");
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionGlow_triggered()
{
    Mlt::Filter* filter = m_model.add(m_isGPU? "movit.glow" : "frei0r.glow");
    if (filter && filter->is_valid()) {
        if (m_isGPU)
            ui->scrollArea->setWidget(new MovitGlowFilter(*filter, true));
    }
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionSharpen_triggered()
{
    m_model.add(m_isGPU? "movit.sharpen" : "frei0r.sharpness");
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionVignette_triggered()
{
    m_model.add(m_isGPU? "movit.vignette" : "vignette");
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_listView_clicked(const QModelIndex &index)
{
    Mlt::Filter* filter = m_model.filterForRow(index.row());
    if (filter && filter->is_valid()) {
        QString name = filter->get("mlt_service");
        if (name == "movit.blur")
            ui->scrollArea->setWidget(new MovitBlurFilter(*filter));
        else if (name == "movit.glow")
            ui->scrollArea->setWidget(new MovitGlowFilter(*filter));
        else
            delete ui->scrollArea->widget();
    }
    delete filter;
}
