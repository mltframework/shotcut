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

#include "docks/recentdock.h"
#include "ui_recentdock.h"
#include <QSettings>
#include <QFileInfo>

static const int MaxItems = 50;

RecentDock::RecentDock(QWidget *parent) :
    Panel(tr("Recent"), parent),
    ui(new Ui::RecentDock)
{
    ui->setupUi(this);
    setWidget(ui->dockWidgetContents);
    m_recent = m_settings.value("recent").toStringList();
    add(QString());
    ui->listWidget->setDragEnabled(true);
    ui->listWidget->setDragDropMode(QAbstractItemView::DragOnly);
}

RecentDock::~RecentDock()
{
    delete ui;
}

QListWidget* RecentDock::listWidget() const
{
    return ui->listWidget;
}

void RecentDock::add(const QString &s)
{
    if (!s.isEmpty()) {
        m_recent.removeOne(s);
        m_recent.prepend(s);
        while (m_recent.count() > MaxItems)
            m_recent.removeLast();
    }
    ui->listWidget->clear();
    foreach (QString s, m_recent) {
        QString name = s;
        if (s.startsWith('/'))
            // Use basename instead.
            name = QFileInfo(s).fileName();
        QListWidgetItem* item  = new QListWidgetItem(name);
        item->setToolTip(s);
        ui->listWidget->addItem(item);
    }
    m_settings.setValue("recent", m_recent);
}

void RecentDock::on_listWidget_itemActivated(QListWidgetItem* i)
{
    emit itemActivated(i->toolTip());
}

void RecentDock::remove(const QString &s)
{
    m_recent.removeOne(s);
    add(QString());
}
