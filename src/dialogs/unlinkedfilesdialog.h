/*
 * Copyright (c) 2016-1029 Meltytech, LLC
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

#ifndef UNLINKEDFILESDIALOG_H
#define UNLINKEDFILESDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QDir>

namespace Ui {
class UnlinkedFilesDialog;
}

class UnlinkedFilesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UnlinkedFilesDialog(QWidget* parent = 0);
    ~UnlinkedFilesDialog();

    void setModel(QStandardItemModel& model);

private slots:
    void on_tableView_doubleClicked(const QModelIndex& index);

    void on_searchFolderButton_clicked();

private:
    bool lookInDir(const QDir& dir, bool recurse = false);

    Ui::UnlinkedFilesDialog *ui;
};

#endif // UNLINKEDFILESDIALOG_H
