/*
 * Copyright (c) 2026 Meltytech, LLC
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

#include "addonfiltersdialog.h"

#include "models/addonservicemodel.h"
#include "widgets/lineeditclear.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

AddOnFiltersDialog::AddOnFiltersDialog(AddOnServiceModel *model, QWidget *parent)
    : QDialog(parent)
    , m_model(model)
    , m_searchField(new LineEditClear(this))
    , m_listWidget(new QTreeWidget(this))
    , m_selectedCountLabel(new QLabel(this))
{
    setWindowTitle(tr("Manage Add-on Filters"));
    resize(760, 560);

    auto *layout = new QVBoxLayout(this);

    auto *description = new QLabel(tr("Select filters to expose as add-on filters."), this);
    description->setWordWrap(true);
    layout->addWidget(description);

    auto *searchLayout = new QHBoxLayout();
    m_searchField->setPlaceholderText(tr("Search service or title"));
    searchLayout->addWidget(m_searchField);
    layout->addLayout(searchLayout);

    m_listWidget->setAlternatingRowColors(true);
    m_listWidget->setRootIsDecorated(false);
    m_listWidget->setUniformRowHeights(true);
    m_listWidget->setSortingEnabled(true);
    m_listWidget->setColumnCount(6);
    m_listWidget->setHeaderLabels(
        {tr("Title"), tr("Service"), tr("Type"), tr("RGBA"), tr("YUV"), tr("10-bit")});
    m_listWidget->header()->setStretchLastSection(true);
    m_listWidget->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_listWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_listWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_listWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_listWidget->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_listWidget->header()->setSectionResizeMode(5, QHeaderView::Stretch);
    m_listWidget->header()->setSortIndicatorShown(true);
    m_listWidget->sortItems(0, Qt::AscendingOrder);
    layout->addWidget(m_listWidget, 1);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(m_selectedCountLabel);
    bottomLayout->addStretch(1);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    bottomLayout->addWidget(buttonBox);
    layout->addLayout(bottomLayout);

    connect(m_searchField, &QLineEdit::textChanged, this, &AddOnFiltersDialog::onSearchTextChanged);
    connect(m_listWidget, &QTreeWidget::itemChanged, this, &AddOnFiltersDialog::onItemChanged);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        onApply();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    populate();
}

void AddOnFiltersDialog::populate()
{
    if (!m_model)
        return;

    m_model->reload();

    const auto enabledServices = m_model->enabledServices();
    m_listWidget->blockSignals(true);
    m_listWidget->setSortingEnabled(false);
    m_listWidget->clear();

    for (int row = 0; row < m_model->rowCount(); ++row) {
        QModelIndex index = m_model->index(row, 0);
        const QString title = index.data(AddOnServiceModel::TitleRole).toString();
        const QString service = index.data(AddOnServiceModel::ServiceRole).toString();
        const QString description = index.data(AddOnServiceModel::DescriptionRole).toString();
        const bool isAudio = index.data(AddOnServiceModel::IsAudioRole).toBool();
        const bool supportsRgba = index.data(AddOnServiceModel::SupportsRgbaRole).toBool();
        const bool supportsYuv = index.data(AddOnServiceModel::SupportsYuvRole).toBool();
        const bool supportsTenBit = index.data(AddOnServiceModel::SupportsTenBitRole).toBool();

        auto *item = new QTreeWidgetItem(m_listWidget);
        item->setText(0, title);
        item->setText(1, service);
        item->setText(2, isAudio ? tr("Audio") : tr("Video"));
        item->setText(3, supportsRgba ? QString(QChar(0x2713)) : QString());
        item->setText(4, supportsYuv ? QString(QChar(0x2713)) : QString());
        item->setText(5, supportsTenBit ? QString(QChar(0x2713)) : QString());
        item->setTextAlignment(3, Qt::AlignCenter);
        item->setTextAlignment(4, Qt::AlignCenter);
        item->setTextAlignment(5, Qt::AlignCenter);
        item->setData(3, Qt::UserRole, supportsRgba ? 1 : 0);
        item->setData(4, Qt::UserRole, supportsYuv ? 1 : 0);
        item->setData(5, Qt::UserRole, supportsTenBit ? 1 : 0);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, enabledServices.contains(service) ? Qt::Checked : Qt::Unchecked);
        item->setData(0, Qt::UserRole, service);
        if (!description.isEmpty()) {
            item->setToolTip(0, description);
            item->setToolTip(1, description);
            item->setToolTip(2, description);
            item->setToolTip(3, description);
            item->setToolTip(4, description);
            item->setToolTip(5, description);
        }
    }

    m_listWidget->setSortingEnabled(true);
    m_listWidget->sortItems(0, Qt::AscendingOrder);
    m_listWidget->blockSignals(false);
    adjustColumnWidths();
    updateSelectedCount();
}

void AddOnFiltersDialog::onSearchTextChanged(const QString &text)
{
    const QString query = text.trimmed();
    for (int i = 0; i < m_listWidget->topLevelItemCount(); ++i) {
        auto *item = m_listWidget->topLevelItem(i);
        const QString service = item->text(1);
        const QString title = item->text(0);
        bool visible = query.isEmpty() || service.contains(query, Qt::CaseInsensitive)
                       || title.contains(query, Qt::CaseInsensitive);
        item->setHidden(!visible);
    }
}

void AddOnFiltersDialog::onItemChanged(QTreeWidgetItem *, int)
{
    updateSelectedCount();
}

void AddOnFiltersDialog::onApply()
{
    if (!m_model)
        return;

    QStringList services;
    for (int i = 0; i < m_listWidget->topLevelItemCount(); ++i) {
        auto *item = m_listWidget->topLevelItem(i);
        if (item->checkState(0) == Qt::Checked)
            services << item->data(0, Qt::UserRole).toString();
    }

    m_model->setEnabledServices(services);
    updateSelectedCount();
}

void AddOnFiltersDialog::updateSelectedCount()
{
    int checked = 0;
    for (int i = 0; i < m_listWidget->topLevelItemCount(); ++i) {
        if (m_listWidget->topLevelItem(i)->checkState(0) == Qt::Checked)
            ++checked;
    }
    m_selectedCountLabel->setText(tr("Selected add-ons: %1").arg(checked));
}

void AddOnFiltersDialog::adjustColumnWidths()
{
    int titleWidth = m_listWidget->fontMetrics().horizontalAdvance(
        m_listWidget->headerItem()->text(0));

    for (int i = 0; i < m_listWidget->topLevelItemCount(); ++i) {
        auto *item = m_listWidget->topLevelItem(i);
        titleWidth = qMax(titleWidth, m_listWidget->fontMetrics().horizontalAdvance(item->text(0)));
    }

    const int kPadding = 32;
    const int kTitleMax = 380;
    const int kTitleMin = 120;
    titleWidth = qBound(kTitleMin, titleWidth + kPadding, kTitleMax);
    m_listWidget->setColumnWidth(0, titleWidth);

    m_listWidget->resizeColumnToContents(1);
    m_listWidget->resizeColumnToContents(2);
    m_listWidget->resizeColumnToContents(3);
    m_listWidget->resizeColumnToContents(4);
    m_listWidget->resizeColumnToContents(5);

    // Ensure dialog is wide enough for all columns.
    const int treeMargins = 24;
    const int dialogMargins = 48;
    const int requiredWidth = m_listWidget->columnWidth(0) + m_listWidget->columnWidth(1)
                              + m_listWidget->columnWidth(2) + m_listWidget->columnWidth(3)
                              + m_listWidget->columnWidth(4) + m_listWidget->columnWidth(5)
                              + treeMargins + dialogMargins;
    if (width() < requiredWidth)
        resize(requiredWidth, height());
    if (minimumWidth() < requiredWidth)
        setMinimumWidth(requiredWidth);
}
