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
#include <QtQml>
#include <QtQuick>
#include <QtAlgorithms>
#include <QActionGroup>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrentRun>
#include "mainwindow.h"
#include "settings.h"
#include "filters/movitglowfilter.h"
#include "filters/frei0rglowfilter.h"
#include "filters/movitsharpenfilter.h"
#include "filters/frei0rsharpnessfilter.h"
#include "filters/whitebalancefilter.h"
#include "filters/webvfxfilter.h"
#include "qmltypes/qmlfilter.h"
#include "qmltypes/qmlmetadata.h"
#include "qmltypes/qmlprofile.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/colorwheelitem.h"

static bool compareQAction(const QAction* a1, const QAction* a2)
{
    return a1->text().toLower() < a2->text().toLower();
}

static QActionList getFilters(FiltersDock* dock, Ui::FiltersDock* ui)
{
    QList<QAction*> actions;
    actions.append(ui->actionGlow);
    actions.append(ui->actionMirror);
#ifndef Q_OS_WIN
    if (!Settings.playerGPU()) actions.append(ui->actionOverlayHTML);
#endif
    actions.append(ui->actionSharpen);
    actions.append(ui->actionWhiteBalance);

    // Find all of the plugin filters.
    qmlRegisterType<QmlMetadata>("org.shotcut.qml", 1, 0, "Metadata");
    qmlRegisterType<ColorWheelItem>("Shotcut.Controls", 1, 0, "ColorWheelItem");
    QQmlEngine engine;
    QDir dir = QmlUtilities::qmlDir();
    dir.cd("filters");
    foreach (QString dirName, dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Executable)) {
        QDir subdir = dir;
        subdir.cd(dirName);
        subdir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        subdir.setNameFilters(QStringList("meta*.qml"));
        foreach (QString fileName, subdir.entryList()) {
            QQmlComponent component(&engine, subdir.absoluteFilePath(fileName));
            QmlMetadata *meta = qobject_cast<QmlMetadata*>(component.create());
            if (meta && (meta->isAudio() || (meta->needsGPU() == Settings.playerGPU()))) {
#ifdef Q_OS_WIN
                if (meta->mlt_service() == "webvfx") {
                    delete meta;
                    continue;
                }
#endif
                // Check if mlt_service is available.
                if (MLT.repository()->filters()->get_data(meta->mlt_service().toLatin1().constData())) {
                    QAction* action = new QAction(meta->name(), 0);
                    meta->setParent(action);
                    meta->setPath(subdir);
                    action->setProperty("isAudio", meta->isAudio());
                    action->setVisible(!meta->isHidden());
                    actions << action;
                    dock->addActionToMap(meta, action);
                }
            } else if (!meta) {
                qWarning() << component.errorString();
            }
        }
    };
    qSort(actions.begin(), actions.end(), compareQAction);
    return actions;
}

FiltersDock::FiltersDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::FiltersDock),
    m_audioActions(0),
    m_videoActions(0),
    m_quickObject(0)
{
    ui->setupUi(this);
    toggleViewAction()->setIcon(windowIcon());
    ui->listView->setModel(&m_model);
    ui->listView->setDragDropMode(QAbstractItemView::InternalMove);
    ui->listView->setDropIndicatorShown(true);
    connect(model(), SIGNAL(changed()), this, SLOT(onModelChanged()));
    m_filtersFuture = QtConcurrent::run(getFilters, this, ui);
}

FiltersDock::~FiltersDock()
{
    foreach (QAction* a, m_filtersFuture.result())
        delete a;
    delete ui;
}

void FiltersDock::availablefilters()
{
    if (!m_videoActions) {
        m_audioActions = new QActionGroup(this);
        m_videoActions = new QActionGroup(this);
        foreach (QAction* action, m_filtersFuture.result()) {
            if (action->property("isAudio").isValid() && action->property("isAudio").toBool())
                m_audioActions->addAction(action);
            else
                m_videoActions->addAction(action);
        }
        connect(m_audioActions, SIGNAL(triggered(QAction*)), SLOT(onActionTriggered(QAction*)));
        connect(m_videoActions, SIGNAL(triggered(QAction*)), SLOT(onActionTriggered(QAction*)));
    }
}

QmlMetadata *FiltersDock::qmlMetadataForService(Mlt::Service *service)
{
    availablefilters();
    if (service->get("shotcut:filter")) {
        QAction* action = m_actionMap.value(service->get("shotcut:filter"));
        if (action && action->children().count())
            return qobject_cast<QmlMetadata*>(action->children().first());
    }
    QAction* action = m_actionMap.value(service->get("mlt_service"));
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
        ui->addAudioButton->setDisabled(true);
        ui->addVideoButton->setDisabled(true);
        MAIN.showStatusMessage(tr("Filters can only be applied to clips."));
    }
    else {
        ui->addAudioButton->setEnabled(true);
        ui->addVideoButton->setEnabled(true);
    }
}

void FiltersDock::setFadeInDuration(int duration)
{
    if (m_quickObject && ui->listView->currentIndex().isValid()) {
        Mlt::Filter* filter = m_model.filterForRow(ui->listView->currentIndex().row());
        if (filter && filter->is_valid()
            && QString(filter->get("shotcut:filter")).startsWith("fadeIn")) {
            m_quickObject->setProperty("duration", duration);
        }
        delete filter;
    }
}

void FiltersDock::setFadeOutDuration(int duration)
{
    if (m_quickObject && ui->listView->currentIndex().isValid()) {
        Mlt::Filter* filter = m_model.filterForRow(ui->listView->currentIndex().row());
        if (filter && filter->is_valid()
            && QString(filter->get("shotcut:filter")).startsWith("fadeOut")) {
            m_quickObject->setProperty("duration", duration);
        }
        delete filter;
    }
}

void FiltersDock::on_addAudioButton_clicked()
{
    availablefilters();
    QPoint pos = ui->addAudioButton->mapToParent(QPoint(0, 0));
    QMenu menu(this);
    menu.addActions(m_audioActions->actions());
    menu.exec(mapToGlobal(pos));
}

void FiltersDock::on_addVideoButton_clicked()
{
    availablefilters();
    QPoint pos = ui->addVideoButton->mapToParent(QPoint(0, 0));
    QMenu menu(this);
    menu.addActions(m_videoActions->actions());
    menu.exec(mapToGlobal(pos));
}

void FiltersDock::on_removeButton_clicked()
{
    QModelIndex index = ui->listView->currentIndex();
    if (index.isValid()) {
        m_model.remove(index.row());
        delete ui->scrollArea->widget();
        ui->scrollArea->setWidget(0);
        m_quickObject = 0;
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
        else if (name == "movit.glow")
            loadWidgetsPanel(new MovitGlowFilter(*filter));
        else if (name == "frei0r.glow")
            loadWidgetsPanel(new Frei0rGlowFilter(*filter));
        else if (name == "movit.sharpen")
            loadWidgetsPanel(new MovitSharpenFilter(*filter));
        else if (name == "frei0r.sharpness")
            loadWidgetsPanel(new Frei0rSharpnessFilter(*filter));
        else if (name == "frei0r.colgate" || name == "movit.white_balance")
            loadWidgetsPanel(new WhiteBalanceFilter(*filter));
        else if (name == "webvfx")
            loadWidgetsPanel(new WebvfxFilter(*filter));
        else
            loadWidgetsPanel();
    }
    delete filter;
}

void FiltersDock::on_actionMirror_triggered()
{
    Mlt::Filter* filter = m_model.add(Settings.playerGPU()? "movit.mirror": "mirror:flip");
    loadWidgetsPanel();
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_listView_doubleClicked(const QModelIndex &index)
{
    m_model.setData(index, true, Qt::CheckStateRole);
}

void FiltersDock::on_actionGlow_triggered()
{
    Mlt::Filter* filter = m_model.add(Settings.playerGPU()? "movit.glow" : "frei0r.glow");
    if (filter && filter->is_valid()) {
        if (Settings.playerGPU())
            loadWidgetsPanel(new MovitGlowFilter(*filter, true));
        else
            loadWidgetsPanel(new Frei0rGlowFilter(*filter, true));
    }
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionSharpen_triggered()
{
    Mlt::Filter* filter = m_model.add(Settings.playerGPU()? "movit.sharpen" : "frei0r.sharpness");
    if (filter && filter->is_valid()) {
        if (Settings.playerGPU())
            loadWidgetsPanel(new MovitSharpenFilter(*filter, true));
        else
            loadWidgetsPanel(new Frei0rSharpnessFilter(*filter, true));
    }
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}

void FiltersDock::on_actionWhiteBalance_triggered()
{
    Mlt::Filter* filter = m_model.add(Settings.playerGPU()? "movit.white_balance": "frei0r.colgate");
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

void FiltersDock::addActionToMap(const QmlMetadata *meta, QAction *action)
{
    m_actionMap[meta->mlt_service()] = action;
    if (!meta->objectName().isEmpty())
        m_actionMap[meta->objectName()] = action;
}

void FiltersDock::loadWidgetsPanel(QWidget *widget)
{
    m_quickObject = 0;
    delete ui->scrollArea->widget();
    ui->scrollArea->setWidget(widget);
}

void FiltersDock::loadQuickPanel(const QmlMetadata* metadata, int row)
{
    if (!metadata) return;
    QQuickView* qqview = new QQuickView;
    QDir importPath = QmlUtilities::qmlDir();
    importPath.cd("modules");
    qqview->engine()->addImportPath(importPath.path());
    QmlFilter* qmlFilter = new QmlFilter(m_model, *metadata, row, qqview);
    qqview->engine()->rootContext()->setContextProperty("filter", qmlFilter);
    QmlProfile* qmlProfile = new QmlProfile(qqview);
    qqview->engine()->rootContext()->setContextProperty("profile", qmlProfile);
    qqview->setResizeMode(QQuickView::SizeRootObjectToView);
    qqview->setColor(palette().window().color());
    qqview->setSource(QUrl::fromLocalFile(metadata->qmlFilePath()));
    QWidget* container = QWidget::createWindowContainer(qqview);
    container->setFocusPolicy(Qt::TabFocus);
    loadWidgetsPanel(container);
    m_quickObject = qqview->rootObject();
}

void FiltersDock::on_actionOverlayHTML_triggered()
{
    Mlt::Filter* filter = m_model.add("webvfx");
    loadWidgetsPanel(new WebvfxFilter(*filter));
    delete filter;
    ui->listView->setCurrentIndex(m_model.index(m_model.rowCount() - 1));
}
