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

#include "edittransactiondialog.h"

#include <QCloseEvent>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

EditTransactionDialog::EditTransactionDialog(QWidget *parent)
    : QDialog(parent, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint)
    , m_label(new QLabel(this))
{
    setWindowTitle(tr("Applying Edits"));
    setWindowModality(Qt::ApplicationModal);
    m_label->setTextFormat(Qt::PlainText);
    m_label->setWordWrap(true);
    auto *progress = new QProgressBar(this);
    progress->setRange(0, 0);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_label);
    layout->addWidget(progress);
    setMinimumWidth(320);
}

void EditTransactionDialog::begin(const QString &text)
{
    m_label->setText(text.isEmpty() ? tr("Applying edits. Please wait.") : text);
    // show() establishes modality without exec() or QProgressDialog's event processing.
    show();
}

void EditTransactionDialog::reject() {}

void EditTransactionDialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
}
