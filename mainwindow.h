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

namespace Ui {
    class MainWindow;
}
class Player;
class RecentDock;
class EncodeDock;
class JobsDock;
class PlaylistDock;
class QUndoStack;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    static MainWindow& singleton();
    ~MainWindow();
    void open(Mlt::Producer* producer);
    bool continueModified();
    QUndoStack* undoStack() const;

signals:
    void producerOpened();

protected:
    MainWindow();
    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent *);
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
    void closeEvent(QCloseEvent*);

private:
    void readSettings();
    void writeSettings();
    void setCurrentFile(const QString &filename);

    Ui::MainWindow* ui;
    QSettings m_settings;
    Player* m_player;
    QDockWidget* m_propertiesDock;
    RecentDock* m_recentDock;
    EncodeDock* m_encodeDock;
    JobsDock* m_jobsDock;
    PlaylistDock* m_playlistDock;
    QString m_currentFile;
    bool m_jobsVisible;
    bool m_isKKeyPressed;
    QUndoStack* m_undoStack;
    QDockWidget* m_historyDock;

public slots:
    void open(const QString& url, const Mlt::Properties* = 0);
    void openVideo();
    void openCut(void*, int in, int out);
    void showStatusMessage(QString);
    void seekPlaylist(int start);
    void onProducerOpened();

private slots:
    void on_actionAbout_Shotcut_triggered();
    void on_actionOpenOther_triggered();
    void onProducerChanged();
    bool on_actionSave_triggered();
    bool on_actionSave_As_triggered();
    void on_actionEncode_triggered(bool checked);
    void onCaptureStateChanged(bool started);
    void onJobsVisibilityChanged(bool checked);
    void onRecentDockTriggered(bool checked);
    void onPropertiesDockTriggered(bool checked);
    void onPlaylistDockTriggered(bool checked);
    void onHistoryDockTriggered(bool checked);
    void onPlaylistCreated();
    void onPlaylistCleared();
    void onPlaylistClosed();
    void onPlaylistModified();
    void onCutModified();
    void updateMarkers();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
};

#define MAIN MainWindow::singleton()

#endif // MAINWINDOW_H
