/*
 * Copyright (c) 2019 Meltytech, LLC
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

#ifndef FILEDATEDIALOG_H
#define FILEDATEDIALOG_H

#include <QDialog>

class QComboBox;
class QDateTimeEdit;
namespace Mlt {
class Producer;
}

class FileDateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileDateDialog(QString title, Mlt::Producer *producer, QWidget *parent = 0);

private slots:
    void accept();
    void dateSelected(int index);

private:
    void populateDateOptions(Mlt::Producer *producer);

    Mlt::Producer *m_producer;
    QComboBox *m_dtCombo;
    QDateTimeEdit *m_dtEdit;
};

#endif // FILEDATEDIALOG_H
