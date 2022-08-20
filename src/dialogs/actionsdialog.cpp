/*
 * Copyright (c) 2022 Meltytech, LLC
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

#include "actionsdialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QAction>
#include <QPushButton>

ActionsDialog::ActionsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Actions and Shortcuts"));
    setSizeGripEnabled(true) ;

    QVBoxLayout *vlayout = new QVBoxLayout();

    // Search Bar
    QHBoxLayout *searchLayout = new QHBoxLayout();
    QLineEdit *searchField = new QLineEdit(this);
    searchField->setPlaceholderText(tr("search"));
    connect(searchField, &QLineEdit::textChanged, this, [&](const QString & text) {
        if (m_proxyModel) {
            m_proxyModel->setFilterRegExp(QRegExp(text, Qt::CaseInsensitive, QRegExp::FixedString));
        }
    });
    connect(searchField, &QLineEdit::returnPressed, this, [&] {
        m_table->setFocus();
        m_table->setCurrentIndex(m_proxyModel->index(0, 0));
    });
    searchLayout->addWidget(searchField);
    QToolButton *clearSearchButton = new QToolButton(this);
    clearSearchButton->setIcon(QIcon::fromTheme("edit-clear",
                                                QIcon(":/icons/oxygen/32x32/actions/edit-clear.png")));
    clearSearchButton->setMaximumSize(22, 22);
    clearSearchButton->setToolTip(tr("Clear search"));
    clearSearchButton->setAutoRaise(true);
    connect(clearSearchButton, &QAbstractButton::clicked, searchField, &QLineEdit::clear);
    searchLayout->addWidget(clearSearchButton);
    vlayout->addLayout(searchLayout);

    // List
    m_table = new QTreeView();
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setItemsExpandable(false);
    m_table->setRootIsDecorated(false);
    m_table->setUniformRowHeights(true);
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(&m_model);
    m_proxyModel->setFilterKeyColumn(0);

    m_table->setModel(m_proxyModel);
    m_table->setWordWrap(false);
    m_table->setSortingEnabled(true);
    m_table->header()->setStretchLastSection(false);
    m_table->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->sortByColumn(ActionsModel::COLUMN_ACTION, Qt::AscendingOrder);
    vlayout->addWidget(m_table);
    // Button Box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    vlayout->addWidget(buttonBox);
    setLayout(vlayout);

    resize(m_table->width() + 100, 600);
    connect(m_table, &QAbstractItemView::activated, this, [&](const QModelIndex & index) {
        auto action = m_model.action(m_proxyModel->mapToSource(index));
        if (action) {
            emit action->triggered();
        }
    });
}
