/*
 * Copyright (c) 2018-2022 Meltytech, LLC
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

#ifndef LISTSELECTIONDIALOG_H
#define LISTSELECTIONDIALOG_H

#include <QDialog>

namespace Ui {
class ListSelectionDialog;
}
class QListWidgetItem;
class QDialogButtonBox;

class ListSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ListSelectionDialog(const QStringList &list, QWidget *parent = 0);
    ~ListSelectionDialog();
    void setColors(const QStringList &colors);
    void setSelection(const QStringList &selection);
    QStringList selection() const;
    QDialogButtonBox *buttonBox() const;

private:
    Ui::ListSelectionDialog *ui;

private slots:
    void onItemActivated(QListWidgetItem *item);
};

#endif // LISTSELECTIONDIALOG_H
