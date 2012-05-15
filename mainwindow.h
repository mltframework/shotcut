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
class Player;
class RecentDock;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void open(Mlt::Producer* producer);

signals:
    void producerOpened();

protected:
    void keyPressEvent(QKeyEvent*);
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
    void closeEvent(QCloseEvent*);
    bool continueModified();

private:
    void readSettings();
    void writeSettings();
    void setCurrentFile(const QString &filename);

    Ui::MainWindow* ui;
    QSettings m_settings;
    Player* m_player;
    QDockWidget* m_propertiesDock;
    RecentDock* m_recentDock;
    QString m_currentFile;

public slots:
    void open(const QString& url, const Mlt::Properties* = 0);
    void openVideo();
    void showStatusMessage(QString);

private slots:
    void on_actionAbout_Shotcut_triggered();
    void on_actionOpenOther_triggered();
    void onProducerOpened();
    void onProducerChanged();
    bool on_actionSave_triggered();
    bool on_actionSave_As_triggered();
};

#endif // MAINWINDOW_H
