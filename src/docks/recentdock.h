/*
 * Copyright (c) 2012-2020 Meltytech, LLC
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
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

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
    void itemActivated(const QString &url);
    void deleted(const QString &url);

public slots:
    void add(const QString &);
    QString remove(const QString &s);
    void find();

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    Ui::RecentDock *ui;
    QStringList m_recent;
    QStandardItemModel m_model;
    QSortFilterProxyModel m_proxyModel;

private slots:
    void on_listWidget_activated(const QModelIndex &i);
    void on_lineEdit_textChanged(const QString &search);
    void on_actionDelete_triggered();
    void on_listWidget_customContextMenuRequested(const QPoint &pos);
};

#endif // RECENTDOCK_H
