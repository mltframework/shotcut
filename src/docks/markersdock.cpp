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
#include "settings.h"
#include "widgets/editmarkerwidget.h"
#include <Logger.h>

#include <QAction>
#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLineEdit>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QSpacerItem>
#include <QTreeView>
#include <QToolButton>
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
  , m_model(nullptr)
  , m_proxyModel(nullptr)
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

    m_addButton = new QToolButton(this);
    m_addButton->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/oxygen/32x32/actions/list-add.png")));
    m_addButton->setMaximumSize(22,22);
    m_addButton->setToolTip(tr("Add a marker at the current time"));
    m_addButton->setAutoRaise(true);
    if (!connect(m_addButton, &QAbstractButton::clicked, this, &MarkersDock::onAddRequested))
         connect(m_addButton, SIGNAL(clicked()), SLOT(onAddRequested()));
    buttonLayout->addWidget(m_addButton);

    m_removeButton = new QToolButton(this);
    m_removeButton->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/oxygen/32x32/actions/list-remove.png")));
    m_removeButton->setMaximumSize(22,22);
    m_removeButton->setToolTip(tr("Remove the selected marker"));
    m_removeButton->setAutoRaise(true);
    if (!connect(m_removeButton, &QAbstractButton::clicked, this, &MarkersDock::onRemoveRequested))
         connect(m_removeButton, SIGNAL(clicked()), SLOT(onRemoveRequested()));
    buttonLayout->addWidget(m_removeButton);

    m_clearButton = new QToolButton(this);
    m_clearButton->setIcon(QIcon::fromTheme("window-close", QIcon(":/icons/oxygen/32x32/actions/window-close.png")));
    m_clearButton->setMaximumSize(22,22);
    m_clearButton->setToolTip(tr("Deselect the marker"));
    m_clearButton->setAutoRaise(true);
    if (!connect(m_clearButton, &QAbstractButton::clicked, this, &MarkersDock::onClearSelectionRequested))
         connect(m_clearButton, SIGNAL(clicked()), SLOT(onClearSelectionRequested()));
    buttonLayout->addWidget(m_clearButton);

    m_moreButton = new QToolButton(this);
    m_moreButton->setIcon(QIcon::fromTheme("show-menu", QIcon(":/icons/oxygen/32x32/actions/show-menu.png")));
    m_moreButton->setMaximumSize(22,22);
    m_moreButton->setToolTip(tr("Display a menu of additional actions"));
    m_moreButton->setAutoRaise(true);
    QMenu* moreMenu = new QMenu(this);
    moreMenu->addAction(tr("Remove all"), this, SLOT(onRemoveAllRequested()));
    QMenu* columnsMenu = new QMenu(tr("Columns"), this);
    QAction* action;
    action = columnsMenu->addAction(tr("Color"), this, SLOT(onColorColumnToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.markersShowColumn("color"));
    action = columnsMenu->addAction(tr("Text"), this, SLOT(onTextColumnToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.markersShowColumn("text"));
    action = columnsMenu->addAction(tr("Start"), this, SLOT(onStartColumnToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.markersShowColumn("start"));
    action = columnsMenu->addAction(tr("End"), this, SLOT(onEndColumnToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.markersShowColumn("end"));
    action = columnsMenu->addAction(tr("Duration"), this, SLOT(onDurationColumnToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.markersShowColumn("duration"));
    moreMenu->addMenu(columnsMenu);
    m_moreButton->setMenu(moreMenu);
    m_moreButton->setPopupMode(QToolButton::QToolButton::InstantPopup);
    buttonLayout->addWidget(m_moreButton);

    m_searchField = new QLineEdit(this);
    m_searchField->setPlaceholderText(tr("search"));
    if (!connect(m_searchField, &QLineEdit::textChanged, this, &MarkersDock::onSearchChanged))
         connect(m_searchField, SIGNAL(textChanged(const QString &)), SLOT(onSearchChanged()));
    buttonLayout->addWidget(m_searchField);

    m_clearSearchButton = new QToolButton(this);
    m_clearSearchButton->setIcon(QIcon::fromTheme("edit-clear", QIcon(":/icons/oxygen/32x32/actions/edit-clear.png")));
    m_clearSearchButton->setMaximumSize(22,22);
    m_clearSearchButton->setToolTip(tr("Clear search"));
    m_clearSearchButton->setAutoRaise(true);
    if (!connect(m_clearSearchButton, &QAbstractButton::clicked, m_searchField, &QLineEdit::clear))
         connect(m_clearSearchButton, SIGNAL(clicked()), m_searchField, SLOT(clear()));
    buttonLayout->addWidget(m_clearSearchButton);

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
    m_proxyModel->setFilterKeyColumn(1);
    m_treeView->setModel(m_proxyModel);
    m_treeView->setColumnHidden(0, !Settings.markersShowColumn("color"));
    m_treeView->setColumnHidden(1, !Settings.markersShowColumn("text"));
    m_treeView->setColumnHidden(2, !Settings.markersShowColumn("start"));
    m_treeView->setColumnHidden(3, !Settings.markersShowColumn("end"));
    m_treeView->setColumnHidden(4, !Settings.markersShowColumn("duration"));
    m_treeView->sortByColumn(Settings.getMarkerSortColumn(), Settings.getMarkerSortOrder());
    connect(m_model, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(onRowsInserted(const QModelIndex&, int, int)));
    connect(m_model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)), this, SLOT(onDataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)));
    connect(m_model, SIGNAL(modelReset()), this, SLOT(onModelReset()));
    connect(m_treeView->header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this, SLOT(onSortIndicatorChanged(int, Qt::SortOrder)));
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
            }
        }
    }
}

void MarkersDock::onClearSelectionRequested()
{
    m_treeView->clearSelection();
}

void MarkersDock::onRemoveAllRequested()
{
    m_model->clear();
}

void MarkersDock::onSearchChanged()
{
    if (m_proxyModel) {
        m_proxyModel->setFilterRegExp(QRegExp(m_searchField->text(), Qt::CaseInsensitive, QRegExp::FixedString));
    }
}

void MarkersDock::onColorColumnToggled(bool checked)
{
    Settings.setMarkersShowColumn("color", checked);
    m_treeView->setColumnHidden(0, !checked);
}

void MarkersDock::onTextColumnToggled(bool checked)
{
    Settings.setMarkersShowColumn("text", checked);
    m_treeView->setColumnHidden(1, !checked);
}

void MarkersDock::onStartColumnToggled(bool checked)
{
    Settings.setMarkersShowColumn("start", checked);
    m_treeView->setColumnHidden(2, !checked);
}

void MarkersDock::onEndColumnToggled(bool checked)
{
    Settings.setMarkersShowColumn("end", checked);
    m_treeView->setColumnHidden(3, !checked);
}

void MarkersDock::onDurationColumnToggled(bool checked)
{
    Settings.setMarkersShowColumn("duration", checked);
    m_treeView->setColumnHidden(4, !checked);
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

void MarkersDock::onModelReset()
{
    m_treeView->clearSelection();
    m_editMarkerWidget->setVisible(false);
}

void MarkersDock::onSortIndicatorChanged(int logicalIndex, Qt::SortOrder order)
{
    Settings.setMarkerSort(logicalIndex, order);
}

void MarkersDock::enableButtons(bool enable)
{
    m_removeButton->setEnabled(enable);
    m_clearButton->setEnabled(enable);
}
