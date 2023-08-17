/*
 * Copyright (c) 2023 Meltytech, LLC
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

#include "resourcewidget.h"

#include "Logger.h"
#include "models/resourcemodel.h"

#include <QHeaderView>
#include <QTreeView>
#include <QVBoxLayout>

ResourceWidget::ResourceWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *vlayout = new QVBoxLayout();

    m_model = new ResourceModel(this);

    m_table = new QTreeView();
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setItemsExpandable(false);
    m_table->setRootIsDecorated(false);
    m_table->setUniformRowHeights(true);
    m_table->setSortingEnabled(false);
    m_table->setModel(m_model);
    m_table->setWordWrap(false);
    m_table->setAlternatingRowColors(true);
    m_table->header()->setStretchLastSection(false);
    qreal rowHeight = fontMetrics().height() * devicePixelRatioF();
    m_table->header()->setMinimumSectionSize(rowHeight);
    m_table->header()->setSectionResizeMode(ResourceModel::COLUMN_INFO, QHeaderView::Fixed);
    m_table->setColumnWidth(ResourceModel::COLUMN_INFO, rowHeight);
    m_table->header()->setSectionResizeMode(ResourceModel::COLUMN_NAME, QHeaderView::Interactive);
    m_table->header()->setSectionResizeMode(ResourceModel::COLUMN_SIZE, QHeaderView::Interactive);
    m_table->header()->setSectionResizeMode(ResourceModel::COLUMN_VID_DESCRIPTION,
                                            QHeaderView::Interactive);
    m_table->header()->setSectionResizeMode(ResourceModel::COLUMN_AUD_DESCRIPTION,
                                            QHeaderView::Interactive);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentChanged, this, [ = ]() {
        m_table->selectionModel()->clearCurrentIndex();
    });
    vlayout->addWidget(m_table);

    setLayout(vlayout);
}

ResourceWidget::~ResourceWidget()
{
}

void ResourceWidget::search(Mlt::Producer *producer)
{
    m_model->search(producer);
}

void ResourceWidget::add(Mlt::Producer *producer)
{
    m_model->add(producer);
}

void ResourceWidget::selectTroubleClips()
{
    m_table->selectionModel()->clearSelection();
    for (int i = 0; i < m_model->rowCount(QModelIndex()); i++) {
        QModelIndex index = m_model->index(i, ResourceModel::COLUMN_INFO);
        if (!m_model->data(index, Qt::ToolTipRole).toString().isEmpty()) {
            m_table->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }
}

bool ResourceWidget::hasTroubleClips()
{
    for (int i = 0; i < m_model->rowCount(QModelIndex()); i++) {
        QModelIndex index = m_model->index(i, ResourceModel::COLUMN_INFO);
        if (!m_model->data(index, Qt::ToolTipRole).toString().isEmpty()) {
            return true;
        }
    }
    return false;
}

QList<Mlt::Producer> ResourceWidget::getSelected()
{
    return m_model->getProducers(m_table->selectionModel()->selectedRows());
}

void ResourceWidget::updateSize()
{
    static const int MAX_COLUMN_WIDTH = 300;
    int tableWidth = 38 + m_table->columnWidth(ResourceModel::COLUMN_INFO);
    for (int i = ResourceModel::COLUMN_NAME; i < m_table->model()->columnCount(); i++) {
        m_table->resizeColumnToContents(i);
        int columnWidth = m_table->columnWidth(i);
        if (columnWidth > MAX_COLUMN_WIDTH) {
            columnWidth = MAX_COLUMN_WIDTH;
            m_table->setColumnWidth(i, columnWidth);
        }
        tableWidth += columnWidth;
    }
    resize(tableWidth, 400);
}
