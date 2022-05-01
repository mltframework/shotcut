/*
 * Copyright (c) 2018 Meltytech, LLC
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

#include "listselectiondialog.h"
#include "ui_listselectiondialog.h"
#include <QListWidget>

ListSelectionDialog::ListSelectionDialog(const QStringList &list, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ListSelectionDialog)
{
    ui->setupUi(this);
    foreach (const QString text, list) {
        QListWidgetItem *item = new QListWidgetItem(text, ui->listWidget);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setCheckState(Qt::Unchecked);
        connect(ui->listWidget, SIGNAL(itemActivated(QListWidgetItem *)),
                SLOT(onItemActivated(QListWidgetItem *)));
    }
}

ListSelectionDialog::~ListSelectionDialog()
{
    delete ui;
}

void ListSelectionDialog::setSelection(const QStringList &selection)
{
    int n = ui->listWidget->count();
    for (int i = 0; i < n; ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (selection.indexOf(item->text()) > -1)
            item->setCheckState(Qt::Checked);
    }
}

QStringList ListSelectionDialog::selection() const
{
    QStringList result;
    int n = ui->listWidget->count();
    for (int i = 0; i < n; ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (item->checkState() == Qt::Checked)
            result << item->text();
    }
    return result;
}

QDialogButtonBox *ListSelectionDialog::buttonBox() const
{
    return ui->buttonBox;
}

void ListSelectionDialog::onItemActivated(QListWidgetItem *item)
{
    item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
}
