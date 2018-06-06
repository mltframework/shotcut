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

#include "docks/recentdock.h"
#include "settings.h"
#include "ui_recentdock.h"
#include "util.h"

#include <QAction>
#include <QDir>
#include <QKeyEvent>
#include <Logger.h>

static const int MaxItems = 100;

RecentDock::RecentDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::RecentDock)
{
    LOG_DEBUG() << "begin";
    ui->setupUi(this);
    toggleViewAction()->setIcon(windowIcon());
    m_recent = Settings.recent();

#ifdef Q_OS_WIN
    // Remove bad entries on Windows due to bug in v17.01.
    QStringList newList;
    bool isRepaired = false;
    foreach (QString s, m_recent) {
        if (s.size() >=3 && s[0] == '/' && s[2] == ':') {
            s.remove(0, 1);
            isRepaired = true;
        }
        newList << s;
    }
    if (isRepaired) {
        m_recent = newList;
        Settings.setRecent(m_recent);
        Settings.sync();
    }
#endif

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
    LOG_DEBUG() << "end";
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

void RecentDock::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
        m_recent.removeAt(ui->listWidget->currentIndex().row());
        Settings.setRecent(m_recent);
        m_model.removeRow(ui->listWidget->currentIndex().row());
    } else {
        QDockWidget::keyPressEvent(event);
    }
}

void RecentDock::on_lineEdit_textChanged(const QString& search)
{
    m_proxyModel.setFilterFixedString(search);
}
