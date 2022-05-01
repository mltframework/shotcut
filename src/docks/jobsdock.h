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

#ifndef JOBSDOCK_H
#define JOBSDOCK_H

#include <QDockWidget>

class AbstractJob;
class QStandardItem;

namespace Ui {
class JobsDock;
}

class JobsDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit JobsDock(QWidget *parent = 0);
    ~JobsDock();
    AbstractJob *currentJob() const;

public slots:
    void onJobAdded();
    void onProgressUpdated(QStandardItem *item, int percent);

protected:
    void resizeEvent(QResizeEvent *event);

private:
    Ui::JobsDock *ui;

private slots:
    void on_treeView_customContextMenuRequested(const QPoint &pos);
    void on_actionStopJob_triggered();
    void on_actionViewLog_triggered();
    void on_pauseButton_toggled(bool checked);
    void on_actionRun_triggered();
    void on_menuButton_clicked();
    void on_treeView_doubleClicked(const QModelIndex &index);
    void on_actionRemove_triggered();
    void on_actionRemoveFinished_triggered();
    void on_JobsDock_visibilityChanged(bool visible);
};

#endif // JOBSDOCK_H
