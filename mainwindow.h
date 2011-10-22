/*
 * Copyright (c) 2011 Meltytech, LLC
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mltcontroller.h"
#ifdef Q_WS_MAC
#   include "glwidget.h"
#endif

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void initializeMlt ();

private:
    void resizeEvent (QResizeEvent* event);
    void forceResize ();

    Ui::MainWindow* ui;
    MltController* mlt;
#ifdef Q_WS_MAC
    GLWidget* gl;
    static void on_frame_show(mlt_consumer, MainWindow*, mlt_frame);
#endif

signals:
    void showImageSignal(QImage);

public slots:
    void openVideo ();
    void play ();
    void pause ();
    void onShowFrame (void* frame, unsigned position);

};

#endif // MAINWINDOW_H
