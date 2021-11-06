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

#include "mainwindow.h"
#include "models/markersmodel.h"
#include "widgets/editmarkerwidget.h"
#include <Logger.h>

#include <QAction>
#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpacerItem>
#include <QTreeView>
#include <QVBoxLayout>
#include <QtWidgets/QScrollArea>

class MarkerTreeView : public QTreeView
{
    Q_OBJECT

public:
    // Make this function public
    using QTreeView::selectedIndexes;

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
  , m_blockSelectionEvent(false)
{
    LOG_DEBUG() << "begin";

    setObjectName("MarkersDock");
    QDockWidget::setWindowTitle(tr("Markers"));
    QIcon filterIcon = QIcon::fromTheme("marker", QIcon(":/icons/oxygen/32x32/actions/marker.png"));
    setWindowIcon(filterIcon);
    toggleViewAction()->setIcon(windowIcon());

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    QDockWidget::setWidget(scrollArea);

    QVBoxLayout* vboxLayout = new QVBoxLayout();
    scrollArea->setLayout(vboxLayout);

    m_treeView = new MarkerTreeView();
    m_treeView->setItemsExpandable(false);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setSortingEnabled(true);
    connect(m_treeView, SIGNAL(markerSelected(QModelIndex&)), this, SLOT(onSelectionChanged(QModelIndex&)));
    vboxLayout->addWidget(m_treeView, 1);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    vboxLayout->addLayout(buttonLayout);

    m_addButton = new QPushButton(this);
    m_addButton->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/oxygen/32x32/actions/list-add.png")));
    m_addButton->setMaximumSize(22,22);
    if (!connect(m_addButton, &QAbstractButton::clicked, this, &MarkersDock::onAddRequested))
         connect(m_addButton, SIGNAL(clicked()), SLOT(onAddRequested()));
    buttonLayout->addWidget(m_addButton);

    m_removeButton = new QPushButton(this);
    m_removeButton->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/oxygen/32x32/actions/list-remove.png")));
    m_removeButton->setMaximumSize(22,22);
    if (!connect(m_removeButton, &QAbstractButton::clicked, this, &MarkersDock::onRemoveRequested))
         connect(m_removeButton, SIGNAL(clicked()), SLOT(onRemoveRequested()));
    buttonLayout->addWidget(m_removeButton);

    m_clearButton = new QPushButton(this);
    m_clearButton->setIcon(QIcon::fromTheme("window-close", QIcon(":/icons/oxygen/32x32/actions/window-close.png")));
    m_clearButton->setMaximumSize(22,22);
    if (!connect(m_clearButton, &QAbstractButton::clicked, this, &MarkersDock::onClearSelectionRequested))
         connect(m_clearButton, SIGNAL(clicked()), SLOT(onClearSelectionRequested()));
    buttonLayout->addWidget(m_clearButton);

    buttonLayout->addStretch();
    enableButtons(false);

    m_editMarkerWidget = new EditMarkerWidget(this, "", "", 0, 0, 0);
    m_editMarkerWidget->setVisible(false);
    connect(m_editMarkerWidget, SIGNAL(valuesChanged()), SLOT(onValuesChanged()));
    vboxLayout->addWidget(m_editMarkerWidget);

    vboxLayout->addStretch();

    LOG_DEBUG() << "end";
}

MarkersDock::~MarkersDock()
{
}

void MarkersDock::setModel(MarkersModel* model)
{
    m_blockSelectionEvent = true;
    m_model = model;
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_treeView->setModel(m_proxyModel);
    connect(m_model, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(onRowsInserted(const QModelIndex&, int, int)));
    connect(m_model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)), this, SLOT(onDataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)));
    m_blockSelectionEvent = false;
}

void MarkersDock::onSelectionChanged(QModelIndex& index)
{
    if (m_blockSelectionEvent == true) return;

    if (m_model && m_proxyModel && MAIN.multitrack() && index.isValid()) {
        QModelIndex realIndex = m_proxyModel->mapToSource(index);
        if (realIndex.isValid()) {
            Markers::Marker marker = m_model->getMarker(realIndex.row());
            emit seekRequested(marker.start);
            enableButtons(true);
            m_editMarkerWidget->setVisible(true);
            QSignalBlocker editBlocker(m_editMarkerWidget);
            m_editMarkerWidget->setValues(marker.text, marker.color, marker.start, marker.end, MAIN.multitrack()->get_length() - 1);
            return;
        }
    }
    m_editMarkerWidget->setVisible(false);
    enableButtons(false);
}

void MarkersDock::onAddRequested()
{
    emit addRequested();
}

void MarkersDock::onRemoveRequested()
{
    if (m_model && m_proxyModel) {
        QModelIndexList indices = m_treeView->selectedIndexes();
        if (indices.size() > 0) {
            QModelIndex realIndex = m_proxyModel->mapToSource(indices[0]);
            if (realIndex.isValid()) {
                m_blockSelectionEvent = true;
                m_model->remove(realIndex.row());
                m_blockSelectionEvent = false;
                m_treeView->clearSelection();
            }
        }
    }
}

void MarkersDock::onClearSelectionRequested()
{
    m_treeView->clearSelection();
}

void MarkersDock::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    QModelIndex sourceIndex = m_model->modelIndexForRow(first);
    QModelIndex insertedIndex = m_proxyModel->mapFromSource(sourceIndex);
    m_treeView->setCurrentIndex(insertedIndex);
}

void MarkersDock::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    if (m_model && m_proxyModel) {
        QModelIndexList indices = m_treeView->selectedIndexes();
        if (indices.size() > 0) {
            QModelIndex realIndex = m_proxyModel->mapToSource(indices[0]);
            if (realIndex.isValid()) {
                Markers::Marker marker = m_model->getMarker(realIndex.row());
                m_editMarkerWidget->setVisible(true);
                QSignalBlocker editBlocker(m_editMarkerWidget);
                m_editMarkerWidget->setValues(marker.text, marker.color, marker.start, marker.end, MAIN.multitrack()->get_length() - 1);
                return;
            }
        }
    }
}

void MarkersDock::onValuesChanged()
{
    if (m_model && m_proxyModel) {
        QModelIndexList indices = m_treeView->selectedIndexes();
        if (indices.size() > 0) {
            QModelIndex realIndex = m_proxyModel->mapToSource(indices[0]);
            if (realIndex.isValid()) {
                Markers::Marker marker;
                marker.text = m_editMarkerWidget->getText();
                marker.color = m_editMarkerWidget->getColor();
                marker.start = m_editMarkerWidget->getStart();
                marker.end = m_editMarkerWidget->getEnd();
                m_model->update(realIndex.row(), marker);
            }
        }
    }
}

void MarkersDock::enableButtons(bool enable)
{
    m_removeButton->setEnabled(enable);
    m_clearButton->setEnabled(enable);
}
