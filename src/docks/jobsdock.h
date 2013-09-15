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

#ifndef JOBSDOCK_H
#define JOBSDOCK_H

#include "widgets/panel.h"

namespace Ui {
    class JobsDock;
}

class JobsDock : public Panel
{
    Q_OBJECT

public:
    explicit JobsDock(QWidget *parent = 0);
    ~JobsDock();

private:
    Ui::JobsDock *ui;

private slots:
    void on_treeView_customContextMenuRequested(const QPoint &pos);
    void on_actionStopJob_triggered();
    void on_actionViewLog_triggered();
    void on_actionViewXml_triggered();
    void on_pauseButton_toggled(bool checked);
    void on_actionOpen_triggered();
    void on_actionRun_triggered();
    void on_menuButton_clicked();
    void on_actionOpenFolder_triggered();
};

#endif // JOBSDOCK_H
