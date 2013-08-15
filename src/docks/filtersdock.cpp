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

#include "filtersdock.h"
#include "ui_filtersdock.h"
#include "mltcontroller.h"
#include <QMenu>
#include <QSettings>
#include "mainwindow.h"
#include "filters/movitblurfilter.h"
#include "filters/movitglowfilter.h"
#include "filters/movitcolorfilter.h"
#include "filters/frei0rcoloradjwidget.h"
#include "filters/boxblurfilter.h"
#include "filters/frei0rglowfilter.h"
#include "filters/cropfilter.h"
#include "filters/saturationfilter.h"
#include "filters/movitsharpenfilter.h"
#include "filters/frei0rsharpnessfilter.h"
#include "filters/whitebalancefilter.h"


FiltersDock::FiltersDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::FiltersDock)
{
    QSettings settings;
    m_isGPU = settings.value("player/gpu", false).toBool();
    ui->setupUi(this);
    toggleViewAction()->setIcon(QIcon::fromTheme("view-filter", windowIcon()));
    ui->listView->setModel(&m_model);
    ui->listView->setDragDropMode(QAbstractItemView::InternalMove);
    ui->listView->setDropIndicatorShown(true);
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
    delete ui->scrollArea->widget();
    m_model.reset();
    onModelChanged();
    if (MLT.isPlaylist() && this->isVisible()) {
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
    menu.addAction(ui->actionColorGrading);
    menu.addAction(ui->actionCrop);
//    if (m_isGPU) menu.addAction(ui->actionDiffusion);
    menu.addAction(ui->actionGlow);
    menu.addAction(ui->actionMirror);
    menu.addAction(ui->actionSaturation);
    menu.addAction(ui->actionSharpen);
//    menu.addAction(ui->actionSizePosition);
//    menu.addAction(ui->actionVignette);
    menu.addAction(ui->actionWhiteBalance);
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

void FiltersDock::on_listView_clicked(const QModelIndex &index)
{
    Mlt::Filter* filter = m_model.filterForRow(index.row());
    if (filter && filter->is_valid()) {
        QString name = filter->get("mlt_service");
        if (name == "movit.blur")
            ui->scrollArea->setWidget(new MovitBlurFilter(*filter));
        else if (name == "movit.glow")
            ui->scrollArea->setWidget(new MovitGlowFilter(*filter));
        else if (name == "movit.lift_gamma_gain")
            ui->scrollArea->setWidget(new MovitColorFilter(*filter));
        else if (name == "frei0r.coloradj_RGB")
            ui->scrollArea->setWidget(new Frei0rColoradjWidget(*filter));
        else if (name == "boxblur")
            ui->scrollArea->setWidget(new BoxblurFilter(*filter));
        else if (name == "frei0r.glow")
            ui->scrollArea->setWidget(new Frei0rGlowFilter(*filter));
        else if (name == "crop")
            ui->scrollArea->setWidget(new CropFilter(*filter));
        else if (name == "frei0r.saturat0r" || name == "movit.saturation")
            ui->scrollArea->setWidget(new SaturationFilter(*filter));
        else if (name == "movit.sharpen")
            ui->scrollArea->setWidget(new MovitSharpenFilter(*filter));
        else if (name == "frei0r.sharpness")
            ui->scrollArea->setWidget(new Frei0rSharpnessFilter(*filter));
        else if (name == "frei0r.colgate" || name == "movit.white_balance")
            ui->scrollArea->setWidget(new WhiteBalanceFilter(*filter));
        else
            delete ui->scrollArea->widget();
    }
    delete filter;
}

void FiltersDock::on_actionBlur_triggered()
{
    Mlt::Filter* filter = m_model.add(m_isGPU? "movit.blur" : "boxblur");
    if (filter && filter->is_valid()) {
        if (m_isGPU)
            ui->scrollArea->setWidget(new MovitBlurFilter(*filter, true));
        else
            ui->scrollArea->setWidget(new BoxblurFilter(*filter, true));
    }
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionMirror_triggered()
{
    Mlt::Filter* filter = m_model.add(m_isGPU? "movit.mirror": "mirror:flip");
    delete ui->scrollArea->widget();
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_listView_doubleClicked(const QModelIndex &index)
{
    m_model.setData(index, true, Qt::CheckStateRole);
}

void FiltersDock::on_actionDiffusion_triggered()
{
    Mlt::Filter* filter = m_model.add("movit.diffusion");
    delete ui->scrollArea->widget();
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionGlow_triggered()
{
    Mlt::Filter* filter = m_model.add(m_isGPU? "movit.glow" : "frei0r.glow");
    if (filter && filter->is_valid()) {
        if (m_isGPU)
            ui->scrollArea->setWidget(new MovitGlowFilter(*filter, true));
        else
            ui->scrollArea->setWidget(new Frei0rGlowFilter(*filter, true));
    }
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionSharpen_triggered()
{
    Mlt::Filter* filter = m_model.add(m_isGPU? "movit.sharpen" : "frei0r.sharpness");
    if (filter && filter->is_valid()) {
        if (m_isGPU)
            ui->scrollArea->setWidget(new MovitSharpenFilter(*filter, true));
        else
            ui->scrollArea->setWidget(new Frei0rSharpnessFilter(*filter, true));
    }
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionVignette_triggered()
{
    Mlt::Filter* filter = m_model.add(m_isGPU? "movit.vignette" : "vignette");
    delete ui->scrollArea->widget();
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionCrop_triggered()
{
    Mlt::Filter* filter = m_model.add("crop");
    ui->scrollArea->setWidget(new CropFilter(*filter, true));
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionColorGrading_triggered()
{
    Mlt::Filter* filter = m_model.add(m_isGPU? "movit.lift_gamma_gain": "frei0r.coloradj_RGB");
    if (filter && filter->is_valid()) {
        if (m_isGPU)
            ui->scrollArea->setWidget(new MovitColorFilter(*filter, true));
        else
            ui->scrollArea->setWidget(new Frei0rColoradjWidget(*filter, true));
    }
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionSizePosition_triggered()
{
    Mlt::Filter* filter = m_model.add(m_isGPU? "movit.rect": "affine");
    delete ui->scrollArea->widget();
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionSaturation_triggered()
{
    Mlt::Filter* filter = m_model.add(m_isGPU? "movit.saturation": "frei0r.saturat0r");
    ui->scrollArea->setWidget(new SaturationFilter(*filter, true));
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionWhiteBalance_triggered()
{
    Mlt::Filter* filter = m_model.add(m_isGPU? "movit.white_balance": "frei0r.colgate");
    ui->scrollArea->setWidget(new WhiteBalanceFilter(*filter, true));
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}
