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

#ifndef EDITTRANSACTIONDIALOG_H
#define EDITTRANSACTIONDIALOG_H

#include <QDialog>

class QLabel;

class EditTransactionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditTransactionDialog(QWidget *parent);
    void begin(const QString &text);
    void reject() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QLabel *m_label;
};

#endif // EDITTRANSACTIONDIALOG_H
