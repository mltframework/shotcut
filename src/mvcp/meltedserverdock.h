/*
 * Copyright (c) 2012-2013 Meltytech, LLC
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

#ifndef MVCPCONSOLEDOCK_H
#define MVCPCONSOLEDOCK_H

#include <QDockWidget>
#include <QModelIndex>
#include <mvcp_remote.h>
#include "mvcpthread.h"

class QConsole;

namespace Ui {
    class MeltedServerDock;
}

class MeltedServerDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit MeltedServerDock(QWidget *parent = 0);
    ~MeltedServerDock();
    QAbstractItemModel* unitsModel() const;
    QAbstractItemModel* clipsModel() const;
    QAction* actionFastForward() const;
    QAction* actionPause() const;
    QAction* actionPlay() const;
    QAction* actionRewind() const;
    QAction* actionStop() const;

signals:
    void connected(MvcpThread*);
    void connected(QString address, quint16 port);
    void disconnected();
    void unitActivated(quint8);
    void unitOpened(quint8);
    void append(QString clip, int in = -1, int out = -1);
    void insert(QString clip, int row, int in = -1, int out = -1);
    void positionUpdated(int position, double fps, int in, int out, int length, bool isPlaying);
    void openLocal(QString resource);

public slots:
    void onAppendRequested();
    void onInsertRequested(int row);

private slots:
    void on_lineEdit_returnPressed();
    void onCommandExecuted(QString);
    void on_connectButton_toggled(bool checked);
    void on_unitsTableView_clicked(const QModelIndex &index);
    void on_unitsTableView_doubleClicked(const QModelIndex &index);
    void on_consoleButton_clicked(bool checked);
    void onPositionUpdated(quint8 unit, int position, double fps, int in, int out, int length, bool isPlaying);

    void on_unitsTableView_customContextMenuRequested(const QPoint &pos);

    void on_actionMapClipsRoot_triggered();

    void on_menuButton_clicked();

    void on_treeView_doubleClicked(const QModelIndex &index);

private:
    Ui::MeltedServerDock *ui;
    QConsole* m_console;
    mvcp_parser m_parser;
    MvcpThread* m_mvcp;
    QString m_mappedClipsRoot;
};

#endif // MVCPCONSOLEDOCK_H
