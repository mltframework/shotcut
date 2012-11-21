/*
 * Copyright (c) 2012 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * GL shader based on BSD licensed code from Peter Bengtsson:
 * http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
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
#include <mvcp_remote.h>
#include <mvcp_util.h>

class QConsole;

namespace Ui {
    class MvcpConsoleDock;
}

class MvcpConsoleDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit MvcpConsoleDock(QWidget *parent = 0);
    ~MvcpConsoleDock();

private slots:
    void on_lineEdit_returnPressed();
    void onCommandExecuted(QString);
    void on_connectButton_toggled(bool checked);

private:
    Ui::MvcpConsoleDock *ui;
    QConsole* m_console;
    mvcp_parser m_parser;
};

#endif // MVCPCONSOLEDOCK_H
