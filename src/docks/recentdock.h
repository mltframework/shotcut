/*
 * Copyright (c) 2012 Meltytech, LLC
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
#include <QSettings>

namespace Ui {
    class RecentDock;
}

class QListWidget;
class QListWidgetItem;

class RecentDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit RecentDock(QWidget *parent = 0);
    ~RecentDock();
    QListWidget* listWidget() const;

signals:
    void itemActivated(const QString& url);

public slots:
    void add(const QString&);
    void remove(const QString& s);

private:
    Ui::RecentDock *ui;
    QSettings m_settings;
    QStringList m_recent;

private slots:
    void on_listWidget_itemActivated(QListWidgetItem* i);
};

#endif // RECENTDOCK_H
