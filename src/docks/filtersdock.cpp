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
#include <QtQml>
#include <QtQuick>
#include <QtAlgorithms>
#include <QActionGroup>
#include <QFileInfo>
#include "mainwindow.h"
#include "filters/movitblurfilter.h"
#include "filters/movitglowfilter.h"
#include "filters/movitcolorfilter.h"
#include "filters/frei0rcoloradjwidget.h"
#include "filters/boxblurfilter.h"
#include "filters/frei0rglowfilter.h"
#include "filters/cropfilter.h"
#include "filters/movitsharpenfilter.h"
#include "filters/frei0rsharpnessfilter.h"
#include "filters/whitebalancefilter.h"
#include "filters/webvfxfilter.h"
#include "qmltypes/qmlfilter.h"
#include "qmltypes/qmlmetadata.h"

static bool compareQAction(const QAction* a1, const QAction* a2)
{
    return a1->text().toLower() < a2->text().toLower();
}

FiltersDock::FiltersDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::FiltersDock),
    m_actions(0)
{
    QSettings settings;
    m_isGPU = settings.value("player/gpu", false).toBool();
    ui->setupUi(this);
    toggleViewAction()->setIcon(QIcon::fromTheme("qqview-filter", windowIcon()));
    ui->listView->setModel(&m_model);
    ui->listView->setDragDropMode(QAbstractItemView::InternalMove);
    ui->listView->setDropIndicatorShown(true);
    connect(model(), SIGNAL(changed()), this, SLOT(onModelChanged()));
}

FiltersDock::~FiltersDock()
{
    delete ui;
}

QActionGroup *FiltersDock::availablefilters()
{
    if (!m_actions) {
        QList<QAction*> actions;
        actions.append(ui->actionBlur);
        actions.append(ui->actionColorGrading);
        actions.append(ui->actionCrop);
    //    if (m_isGPU) menu.append(ui->actionDiffusion);
        actions.append(ui->actionGlow);
        actions.append(ui->actionMirror);
        if (!m_isGPU) actions.append(ui->actionOverlayHTML);
        actions.append(ui->actionSharpen);
    //    menu.append(ui->actionSizePosition);
    //    menu.append(ui->actionVignette);
        actions.append(ui->actionWhiteBalance);

        // Find all of the plugin filters.
        qmlRegisterType<QmlMetadata>("org.shotcut.qml", 1, 0, "Metadata");
        QQmlEngine engine;
        QDir dir = qmlDir();
        dir.cd("filters");
        foreach (QString dirName, dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Executable)) {
            QDir subdir = dir;
            subdir.cd(dirName);
            subdir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
            subdir.setNameFilters(QStringList("meta*.qml"));
            foreach (QString fileName, subdir.entryList()) {
                QQmlComponent component(&engine, subdir.absoluteFilePath(fileName));
                QmlMetadata *meta = qobject_cast<QmlMetadata*>(component.create());
                if (meta && (meta->isAudio() || (meta->needsGPU() == m_isGPU))) {
                    // Check if mlt_service is available.
                    if (MLT.repository()->filters()->get_data(meta->mlt_service().toLatin1().constData())) {
                        QAction* action = new QAction(meta->name(), this);
                        meta->setParent(action);
                        meta->setPath(subdir);
                        actions << action;
                        if (!meta->objectName().isEmpty())
                            m_objectNameActionMap[meta->objectName()] = action;
                        else
                            m_serviceActionMap[meta->mlt_service()] = action;
                    }
                } else if (!meta) {
                    qWarning() << component.errorString();
                }
            }
        };
        qSort(actions.begin(), actions.end(), compareQAction);
        m_actions = new QActionGroup(this);
        foreach (QAction* action, actions)
            m_actions->addAction(action);
        connect(m_actions, SIGNAL(triggered(QAction*)), SLOT(onActionTriggered(QAction*)));
    }
    return m_actions;
}

QmlMetadata *FiltersDock::qmlMetadataForService(Mlt::Service *service)
{
    availablefilters();
    if (service->get("shotcut:filter")) {
        QAction* action = m_objectNameActionMap.value(service->get("shotcut:filter"));
        if (action && action->children().count())
            return qobject_cast<QmlMetadata*>(action->children().first());
    }
    QAction* action = m_serviceActionMap.value(service->get("mlt_service"));
    if (action && action->children().count())
        return qobject_cast<QmlMetadata*>(action->children().first());
    else
        return 0;
}

void FiltersDock::onModelChanged()
{
    MLT.refreshConsumer();
    ui->removeButton->setEnabled(m_model.rowCount() > 0);
}

void FiltersDock::onProducerOpened()
{
    loadWidgetsPanel();
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
    menu.addActions(availablefilters()->actions());
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
        QmlMetadata* meta = qmlMetadataForService(filter);
        if (meta)
            loadQuickPanel(meta, index.row());
        else if (name == "movit.blur")
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
        else if (name == "movit.sharpen")
            ui->scrollArea->setWidget(new MovitSharpenFilter(*filter));
        else if (name == "frei0r.sharpness")
            ui->scrollArea->setWidget(new Frei0rSharpnessFilter(*filter));
        else if (name == "frei0r.colgate" || name == "movit.white_balance")
            ui->scrollArea->setWidget(new WhiteBalanceFilter(*filter));
        else if (name == "webvfx")
            ui->scrollArea->setWidget(new WebvfxFilter(*filter));
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

void FiltersDock::on_actionWhiteBalance_triggered()
{
    Mlt::Filter* filter = m_model.add(m_isGPU? "movit.white_balance": "frei0r.colgate");
    loadWidgetsPanel(new WhiteBalanceFilter(*filter, true));
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::onActionTriggered(QAction* action)
{
    if (action->children().count()) {
        loadQuickPanel(qobject_cast<QmlMetadata*>(action->children().first()));
        ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
    }
}

QDir FiltersDock::qmlDir() const
{
    QDir dir(qApp->applicationDirPath());
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    dir.cdUp();
#endif
    dir.cd("share");
    dir.cd("shotcut");
    dir.cd("qml");
    return dir;
}

void FiltersDock::loadWidgetsPanel(QWidget *widget)
{
    delete ui->scrollArea->widget();
    ui->scrollArea->setWidget(widget);
}

void FiltersDock::loadQuickPanel(const QmlMetadata* metadata, int row)
{
    if (!metadata) return;
    QQuickView* qqview = new QQuickView;
    QDir importPath = qmlDir();
    importPath.cd("modules");
    qqview->engine()->addImportPath(importPath.path());
    QmlFilter* qmlFilter = new QmlFilter(m_model, *metadata, row, qqview);
    qqview->engine()->rootContext()->setContextProperty("filter", qmlFilter);
    qqview->setResizeMode(QQuickView::SizeRootObjectToView);
    qqview->setColor(palette().window().color());
    qqview->setSource(QUrl::fromLocalFile(metadata->qmlFilePath()));
    QWidget* container = QWidget::createWindowContainer(qqview);
    container->setFocusPolicy(Qt::TabFocus);
    loadWidgetsPanel(container);
}

void FiltersDock::on_actionOverlayHTML_triggered()
{
    Mlt::Filter* filter = m_model.add("webvfx");
    ui->scrollArea->setWidget(new WebvfxFilter(*filter));
    // This is needed for webvfx to prevent it from hanging app.
    filter->set("consumer", MLT.consumer()->get_service(), 0);
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}
