/*
 * Copyright (c) 2012-2018 Meltytech, LLC
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

#ifndef OPENOTHERDIALOG_H
#define OPENOTHERDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>

namespace Ui {
class OpenOtherDialog;
}
namespace Mlt {
class Properties;
class Producer;
class Profile;
}

class OpenOtherDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenOtherDialog(QWidget *parent = 0);
    ~OpenOtherDialog();

    Mlt::Producer *newProducer(Mlt::Profile &) const;
    void load(Mlt::Producer *);

private slots:
    void on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    Ui::OpenOtherDialog *ui;
    QObject *m_current;

    Mlt::Producer *newProducer(Mlt::Profile &, QObject *widget) const;
    void selectTreeWidget(const QString &s);
};

#endif // OPENOTHERDIALOG_H
