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

    LOG_DEBUG() << "Create Resource Model";

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
    m_table->header()->setStretchLastSection(false);
    m_table->header()->setSectionResizeMode(ResourceModel::COLUMN_NAME, QHeaderView::ResizeToContents);
    m_table->header()->setSectionResizeMode(ResourceModel::COLUMN_SIZE, QHeaderView::ResizeToContents);
    m_table->header()->setSectionResizeMode(ResourceModel::COLUMN_VID_DESCRIPTION,
                                            QHeaderView::ResizeToContents);
    m_table->header()->setSectionResizeMode(ResourceModel::COLUMN_AUD_DESCRIPTION,
                                            QHeaderView::ResizeToContents);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentChanged, this, [ = ]() {
        m_table->selectionModel()->clearCurrentIndex();
    });
    vlayout->addWidget(m_table);

    setLayout(vlayout);

    int tableWidth = 38;
    for (int i = 0; i < m_table->model()->columnCount(); i++) {
        tableWidth += m_table->columnWidth(i);
    }
    resize(tableWidth, 400);
}

ResourceWidget::~ResourceWidget()
{
}

QList<Mlt::Producer> ResourceWidget::getSelected()
{
    return m_model->getProducers(m_table->selectionModel()->selectedRows());
}
