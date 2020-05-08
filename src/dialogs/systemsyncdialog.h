/*
 * Copyright (c) 2020 Meltytech, LLC
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

#ifndef SYSTEMSYNCDIALOG_H
#define SYSTEMSYNCDIALOG_H

#include <QDialog>

namespace Ui {
class SystemSyncDialog;
}

class SystemSyncDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SystemSyncDialog(QWidget *parent = nullptr);
    ~SystemSyncDialog();

private slots:
    void on_syncSlider_sliderReleased();

    void on_syncSpinBox_editingFinished();

    void on_buttonBox_rejected();

    void on_undoButton_clicked();

    void on_syncSpinBox_valueChanged(int arg1);

    void on_applyButton_clicked();

private:
    Ui::SystemSyncDialog *ui;
    int m_oldValue;

    void setDelay(int delay);
};

#endif // SYSTEMSYNCDIALOG_H
