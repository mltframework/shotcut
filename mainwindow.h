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
#include <QSettings>
#include "mltcontroller.h"
#ifdef Q_WS_MAC
#   include "glwidget.h"
#endif

namespace Ui {
    class MainWindow;
}
class ScrubBar;
class QSpinBox;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void open(const QString& url);

protected:
    void keyPressEvent(QKeyEvent*);
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
    void closeEvent(QCloseEvent*);

private:
    void resizeEvent(QResizeEvent* event);
    void forceResize();
    void readSettings();
    void writeSettings();

    Ui::MainWindow* ui;
    Mlt::Controller* mltWidget;
    QIcon m_playIcon;
    QIcon m_pauseIcon;
    QSettings m_settings;
    ScrubBar* m_scrubber;
    QSpinBox* m_positionSpinner;
    QLabel* m_durationLabel;

public slots:
    void openVideo();
    void togglePlayPause();
    void play(double speed = 1.0);
    void pause();
    void onShowFrame(Mlt::QFrame frame, unsigned position);
    void onSeek(int position);
    void onInChanged(int in);
    void onOutChanged(int out);
    void onVideoWidgetContextMenu(const QPoint& pos);

private slots:
    void on_actionAbout_Shotcut_triggered();
    void on_actionOpenURL_triggered();
    void on_actionSkipNext_triggered();
    void on_actionSkipPrevious_triggered();
    void on_actionProgressive_triggered(bool checked);
    void on_actionLowQuality_triggered(bool checked);
    void on_actionMediumQuality_triggered(bool checked);
    void on_actionHighQuality_triggered(bool checked);
};

#endif // MAINWINDOW_H
