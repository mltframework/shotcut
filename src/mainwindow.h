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
#ifdef WITH_LIBLEAP
#include "leaplistener.h"
#endif

namespace Ui {
    class MainWindow;
}
class Player;
class RecentDock;
class EncodeDock;
class JobsDock;
class PlaylistDock;
class QUndoStack;
class MeltedPlaylistDock;
class MeltedServerDock;
class QActionGroup;
class FiltersDock;
class HtmlEditor;
class TimelineDock;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    static MainWindow& singleton();
    ~MainWindow();
    void open(Mlt::Producer* producer);
    bool continueModified();
    QUndoStack* undoStack() const;
    void saveXML(const QString& filename);
    static void changeTheme(const QString& theme);
    FiltersDock* filtersDock() const { return m_filtersDock; }
    HtmlEditor* htmlEditor() const { return m_htmlEditor; }

signals:
    void producerOpened();
    void profileChanged();
    void openFailed(QString);

protected:
    MainWindow();
    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent *);
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
    void closeEvent(QCloseEvent*);

private:
    void setupSettingsMenu();
    QAction *addProfile(QActionGroup* actionGroup, const QString& desc, const QString& name);
    void readPlayerSettings();
    void readWindowSettings();
    void writeSettings();
    void configureVideoWidget();
    void setCurrentFile(const QString &filename);
    void changeDeinterlacer(bool checked, const char* method);
    void changeInterpolation(bool checked, const char* method);

    Ui::MainWindow* ui;
    Player* m_player;
    QDockWidget* m_propertiesDock;
    RecentDock* m_recentDock;
    EncodeDock* m_encodeDock;
    JobsDock* m_jobsDock;
    PlaylistDock* m_playlistDock;
    TimelineDock* m_timelineDock;
    QString m_currentFile;
    bool m_jobsVisible;
    bool m_isKKeyPressed;
    QUndoStack* m_undoStack;
    QDockWidget* m_historyDock;
    MeltedServerDock* m_meltedServerDock;
    MeltedPlaylistDock* m_meltedPlaylistDock;
    QActionGroup* m_profileGroup;
    QActionGroup* m_externalGroup;
    QActionGroup* m_keyerGroup;
    FiltersDock* m_filtersDock;
    QMenu* m_customProfileMenu;
    QMenu* m_keyerMenu;
    QStringList m_multipleFiles;
    bool m_isPlaylistLoaded;
    QActionGroup* m_languagesGroup;
    HtmlEditor* m_htmlEditor;
#ifdef WITH_LIBLEAP
    LeapListener m_leapListener;
#endif

public slots:
    void open(const QString& url, const Mlt::Properties* = 0);
    void openVideo();
    void openCut(void*, int in, int out);
    void showStatusMessage(QString);
    void seekPlaylist(int start);
    void onProducerOpened();
    void onGpuNotSupported();
    void editHTML(const QString& fileName);
    void stepLeftOneFrame();
    void stepRightOneFrame();
    void stepLeftOneSecond();
    void stepRightOneSecond();
    void setInToCurrent();
    void setOutToCurrent();
    void onShuttle(float x);

private slots:
    void on_actionAbout_Shotcut_triggered();
    void on_actionOpenOther_triggered();
    void onProducerChanged();
    bool on_actionSave_triggered();
    bool on_actionSave_As_triggered();
    void onEncodeTriggered(bool checked = true);
    void onCaptureStateChanged(bool started);
    void onEncodeVisibilityChanged(bool);
    void onJobsVisibilityChanged(bool);
    void onRecentDockTriggered(bool checked = true);
    void onPropertiesDockTriggered(bool checked = true);
    void onPlaylistDockTriggered(bool checked = true);
    void onTimelineDockTriggered(bool checked = true);
    void onHistoryDockTriggered(bool checked = true);
    void onFiltersDockTriggered(bool checked = true);
    void onPlaylistCreated();
    void onPlaylistCleared();
    void onPlaylistClosed();
    void onPlaylistModified();
    void onCutModified();
    void updateMarkers();
    void updateThumbnails();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionFAQ_triggered();
    void on_actionForum_triggered();
    void onMeltedUnitOpened();
    void onMeltedUnitActivated();
    void on_actionEnter_Full_Screen_triggered();
    void on_actionOpenGL_triggered(bool checked);
    void on_actionRealtime_triggered(bool checked);
    void on_actionProgressive_triggered(bool checked);
    void on_actionOneField_triggered(bool checked);
    void on_actionLinearBlend_triggered(bool checked);
    void on_actionYadifTemporal_triggered(bool checked);
    void on_actionYadifSpatial_triggered(bool checked);
    void on_actionNearest_triggered(bool checked);
    void on_actionBilinear_triggered(bool checked);
    void on_actionBicubic_triggered(bool checked);
    void on_actionHyper_triggered(bool checked);
    void on_actionJack_triggered(bool checked);
    void on_actionGPU_triggered(bool checked);
    void onExternalTriggered(QAction*);
    void onKeyerTriggered(QAction*);
    void onProfileTriggered(QAction*);
    void on_actionAddCustomProfile_triggered();
    void processMultipleFiles();
    void onLanguageTriggered(QAction*);
    void on_actionSystemTheme_triggered();
    void on_actionFusionDark_triggered();
    void on_actionFusionLight_triggered();
    void on_actionTutorials_triggered();
};

#define MAIN MainWindow::singleton()

#endif // MAINWINDOW_H
