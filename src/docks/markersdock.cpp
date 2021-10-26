/*
 * Copyright (c) 2021 Meltytech, LLC
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

#include "markersdock.h"

#include "models/markersmodel.h"
#include <Logger.h>

#include <QAction>
#include <QDebug>
#include <QHeaderView>
#include <QIcon>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QtWidgets/QScrollArea>

class MarkerTreeView : public QTreeView
{
    Q_OBJECT

signals:
    void markerSelected(QModelIndex& index);

protected:
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        QModelIndex signalIndex;
        QTreeView::selectionChanged(selected, deselected);
        QModelIndexList indices = selectedIndexes();
        if (indices.size() > 0) {
            signalIndex = indices[0];
        }
        emit markerSelected(signalIndex);
    }
};

// Include this so that MarkerTreeView can be declared in the source file.
#include "markersdock.moc"

MarkersDock::MarkersDock(QWidget *parent) :
    QDockWidget(parent)
{
    LOG_DEBUG() << "begin";

    setObjectName("MarkersDock");
    QIcon filterIcon = QIcon::fromTheme("marker", QIcon(":/icons/oxygen/32x32/actions/marker.png"));
    setWindowIcon(filterIcon);
    toggleViewAction()->setIcon(windowIcon());

    m_treeView = new MarkerTreeView();
    m_treeView->setItemsExpandable(false);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setSortingEnabled(true);
    connect(m_treeView, SIGNAL(markerSelected(QModelIndex&)), this, SLOT(onSelectionChanged(QModelIndex&)));

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(m_treeView);
    QDockWidget::setWidget(scrollArea);
    QDockWidget::setWindowTitle(tr("Markers"));

    LOG_DEBUG() << "end";
}

MarkersDock::~MarkersDock()
{
}

void MarkersDock::setModel(MarkersModel* model)
{
    m_model = model;
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_treeView->setModel(m_proxyModel);
}

void MarkersDock::onSelectionChanged(QModelIndex& index)
{
    if (m_model && m_proxyModel && index.isValid()) {
        QModelIndex realIndex = m_proxyModel->mapToSource(index);
        Markers::Marker marker = m_model->getMarker(realIndex.row());
        emit seekRequested(marker.start);
    }
}
