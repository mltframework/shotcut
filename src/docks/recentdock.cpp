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
#include "settings.h"
#include "ui_recentdock.h"
#include "util.h"

#include <QDir>
#include <QDebug>

static const int MaxItems = 100;

RecentDock::RecentDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::RecentDock)
{
    qDebug() << "begin";
    ui->setupUi(this);
    toggleViewAction()->setIcon(windowIcon());
    m_recent = Settings.recent();
    ui->listWidget->setDragEnabled(true);
    ui->listWidget->setDragDropMode(QAbstractItemView::DragOnly);
    foreach (QString s, m_recent) {
        QStandardItem* item = new QStandardItem(Util::baseName(s));
        item->setToolTip(s);
        m_model.appendRow(item);
    }
    m_proxyModel.setSourceModel(&m_model);
    m_proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->listWidget->setModel(&m_proxyModel);
    qDebug() << "end";
}

RecentDock::~RecentDock()
{
    delete ui;
}

void RecentDock::add(const QString &s)
{
    if (s.startsWith(QDir::tempPath())) return;
    QString name = remove(s);
    QStandardItem* item = new QStandardItem(name);
    item->setToolTip(s);
    m_model.insertRow(0, item);
    m_recent.prepend(s);
    while (m_recent.count() > MaxItems)
        m_recent.removeLast();
    Settings.setRecent(m_recent);
}

void RecentDock::on_listWidget_activated(const QModelIndex& i)
{
    ui->listWidget->setCurrentIndex(QModelIndex());
    emit itemActivated(m_proxyModel.itemData(i)[Qt::ToolTipRole].toString());
}

QString RecentDock::remove(const QString &s)
{
    m_recent.removeOne(s);
    Settings.setRecent(m_recent);

    QString name = Util::baseName(s);
    QList<QStandardItem*> items = m_model.findItems(name);
    if (items.count() > 0)
        m_model.removeRow(items.first()->row());
    return name;
}

void RecentDock::on_lineEdit_textChanged(const QString& search)
{
    m_proxyModel.setFilterFixedString(search);
}
