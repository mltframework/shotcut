/*
 * Copyright (c) 2012-2017 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#ifndef RECENTDOCK_H
#define RECENTDOCK_H

#include <QDockWidget>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

namespace Ui {
    class RecentDock;
}

class RecentDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit RecentDock(QWidget *parent = 0);
    ~RecentDock();

signals:
    void itemActivated(const QString& url);

public slots:
    void add(const QString&);
    QString remove(const QString& s);

protected:
    void keyPressEvent(QKeyEvent* event);

private:
    Ui::RecentDock *ui;
    QStringList m_recent;
    QStandardItemModel m_model;
    QSortFilterProxyModel m_proxyModel;

private slots:
    void on_listWidget_activated(const QModelIndex& i);
    void on_lineEdit_textChanged(const QString& search);
};

#endif // RECENTDOCK_H
