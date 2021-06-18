/*
 * Copyright (c) 2011-2021 Meltytech, LLC
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
#include <QMutex>
#include <QTimer>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QScopedPointer>
#include <QSharedPointer>
#include "mltcontroller.h"
#include "mltxmlchecker.h"

#define EXIT_RESTART (42)

namespace Ui {
    class MainWindow;
}
class Player;
class RecentDock;
class EncodeDock;
class JobsDock;
class PlaylistDock;
class QUndoStack;
class QActionGroup;
class FilterController;
class ScopeController;
class FiltersDock;
class TimelineDock;
class AutoSaveFile;
class QNetworkReply;
class KeyframesDock;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum LayoutMode {
        Custom = 0,
        Logging,
        Editing,
        Effects,
        Color,
        Audio,
        PlayerOnly
    };

    static MainWindow& singleton();
    ~MainWindow();
    void open(Mlt::Producer* producer);
    bool continueModified();
    bool continueJobsRunning();
    QUndoStack* undoStack() const;
    bool saveXML(const QString& filename, bool withRelativePaths = true);
    static void changeTheme(const QString& theme);
    PlaylistDock* playlistDock() const { return m_playlistDock; }
    FilterController* filterController() const { return m_filterController; }
    Mlt::Playlist* playlist() const;
    bool isPlaylistValid() const;
    Mlt::Producer* multitrack() const;
    bool isMultitrackValid() const;
    void doAutosave();
    void setFullScreen(bool isFullScreen);
    QString removeFileScheme(QUrl& url);
    QString untitledFileName() const;
    void setProfile(const QString& profile_name);
    QString fileName() const { return m_currentFile; }
    bool isSourceClipMyProject(QString resource = MLT.resource(), bool withDialog = true);
    bool keyframesDockIsVisible() const;

    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent *);
    void hideSetDataDirectory();
    QMenu* customProfileMenu() const { return m_customProfileMenu; }
    QAction* actionAddCustomProfile() const;
    QAction* actionProfileRemove() const;
    QActionGroup* profileGroup() const { return m_profileGroup; }
    void buildVideoModeMenu(QMenu *topMenu, QMenu *&customMenu, QActionGroup* group, QAction *addAction, QAction *removeAction);
    void newProject(const QString& filename, bool isProjectFolder = false);
    void addCustomProfile(const QString& name, QMenu* menu, QAction* action, QActionGroup* group);
    void removeCustomProfiles(const QStringList& profiles, QDir &dir, QMenu* menu, QAction* action);
    QUuid timelineClipUuid(int trackIndex, int clipIndex);
    void replaceInTimeline(const QUuid& uuid, Mlt::Producer& producer);
    Mlt::ClipInfo* timelineClipInfoByUuid(const QUuid& uuid, int& trackIndex, int& clipIndex);
    void replaceAllByHash(const QString& hash, Mlt::Producer& producer, bool isProxy = false);

signals:
    void audioChannelsChanged();
    void producerOpened(bool withReopen = true);
    void profileChanged();
    void openFailed(QString);
    void aboutToShutDown();
    void renameRequested();
    void serviceInChanged(int delta, Mlt::Service*);
    void serviceOutChanged(int delta, Mlt::Service*);

protected:
    MainWindow();
    bool eventFilter(QObject* target, QEvent* event);
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
    void closeEvent(QCloseEvent*);
    void showEvent(QShowEvent*);

private:
    void setupSettingsMenu();
    void setupOpenOtherMenu();
    QAction *addProfile(QActionGroup* actionGroup, const QString& desc, const QString& name);
    QAction *addLayout(QActionGroup* actionGroup, const QString& name);
    void readPlayerSettings();
    void readWindowSettings();
    void writeSettings();
    void configureVideoWidget();
    void setCurrentFile(const QString &filename);
    void changeAudioChannels(bool checked, int channels);
    void changeDeinterlacer(bool checked, const char* method);
    void changeInterpolation(bool checked, const char* method);
    bool checkAutoSave(QString &url);
    void stepLeftBySeconds(int sec);
    bool saveRepairedXmlFile(MltXmlChecker& checker, QString& fileName);
    void setAudioChannels(int channels);
    void showSaveError();
    void setPreviewScale(int scale);
    void setVideoModeMenu();
    void resetVideoModeMenu();
    void resetDockCorners();
    void showIncompatibleProjectMessage(const QString& shotcutVersion);
    void restartAfterChangeTheme();

    Ui::MainWindow* ui;
    Player* m_player;
    QDockWidget* m_propertiesDock;
    RecentDock* m_recentDock;
    EncodeDock* m_encodeDock;
    JobsDock* m_jobsDock;
    PlaylistDock* m_playlistDock;
    TimelineDock* m_timelineDock;
    QString m_currentFile;
    bool m_isKKeyPressed;
    QUndoStack* m_undoStack;
    QDockWidget* m_historyDock;
    QActionGroup* m_profileGroup;
    QActionGroup* m_externalGroup;
    QActionGroup* m_keyerGroup;
    QActionGroup* m_layoutGroup;
    QActionGroup* m_previewScaleGroup;
    FiltersDock* m_filtersDock;
    FilterController* m_filterController;
    ScopeController* m_scopeController;
    QMenu* m_customProfileMenu;
    QMenu* m_keyerMenu;
    QStringList m_multipleFiles;
    bool m_isPlaylistLoaded;
    QActionGroup* m_languagesGroup;
    QSharedPointer<AutoSaveFile> m_autosaveFile;
    QMutex m_autosaveMutex;
    QTimer m_autosaveTimer;
    int m_exitCode;
    int m_navigationPosition;
    QScopedPointer<QAction> m_statusBarAction;
    QNetworkAccessManager m_network;
    QString m_upgradeUrl;
    KeyframesDock* m_keyframesDock;

#ifdef WITH_LIBLEAP
    LeapListener m_leapListener;
#endif

public slots:
    bool isCompatibleWithGpuMode(MltXmlChecker& checker);
    bool isXmlRepaired(MltXmlChecker& checker, QString& fileName);
    void open(QString url, const Mlt::Properties* = nullptr, bool play = true);
    void openMultiple(const QStringList& paths);
    void openMultiple(const QList<QUrl>& urls);
    void openVideo();
    void openCut(Mlt::Producer* producer, bool play = false);
    void hideProducer();
    void closeProducer();
    void showStatusMessage(QAction* action, int timeoutSeconds = 5);
    void showStatusMessage(const QString& message, int timeoutSeconds = 5);
    void seekPlaylist(int start);
    void seekTimeline(int position, bool seekPlayer = true);
    void seekKeyframes(int position);
    QWidget* loadProducerWidget(Mlt::Producer* producer);
    void onProducerOpened(bool withReopen = true);
    void onGpuNotSupported();
    void stepLeftOneFrame();
    void stepRightOneFrame();
    void stepLeftOneSecond();
    void stepRightOneSecond();
    void setInToCurrent(bool ripple);
    void setOutToCurrent(bool ripple);
    void onShuttle(float x);
    void onPropertiesDockTriggered(bool checked = true);
    bool on_actionSave_triggered();

private slots:
    void showUpgradePrompt();
    void on_actionAbout_Shotcut_triggered();
    void on_actionOpenOther_triggered();
    void onProducerChanged();
    bool on_actionSave_As_triggered();
    void onEncodeTriggered(bool checked = true);
    void onCaptureStateChanged(bool started);
    void onJobsDockTriggered(bool = true);
    void onRecentDockTriggered(bool checked = true);
    void onPlaylistDockTriggered(bool checked = true);
    void onTimelineDockTriggered(bool checked = true);
    void onHistoryDockTriggered(bool checked = true);
    void onFiltersDockTriggered(bool checked = true);
    void onKeyframesDockTriggered(bool checked = true);
    void onPlaylistCreated();
    void onPlaylistLoaded();
    void onPlaylistCleared();
    void onPlaylistClosed();
    void onPlaylistModified();
    void onMultitrackCreated();
    void onMultitrackClosed();
    void onMultitrackModified();
    void onMultitrackDurationChanged();
    void onCutModified();
    void onProducerModified();
    void onFilterModelChanged();
    void updateMarkers();
    void updateThumbnails();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionFAQ_triggered();
    void on_actionForum_triggered();
    void on_actionEnter_Full_Screen_triggered();
    void on_actionRealtime_triggered(bool checked);
    void on_actionProgressive_triggered(bool checked);
    void on_actionChannels1_triggered(bool checked);
    void on_actionChannels2_triggered(bool checked);
    void on_actionChannels6_triggered(bool checked);
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
    void onProfileChanged();
    void on_actionAddCustomProfile_triggered();
    void processMultipleFiles();
    void onLanguageTriggered(QAction*);
    void on_actionSystemTheme_triggered();
    void on_actionFusionDark_triggered();
    void on_actionFusionLight_triggered();
    void on_actionTutorials_triggered();
    void on_actionRestoreLayout_triggered();
    void on_actionShowTitleBars_triggered(bool checked);
    void on_actionShowToolbar_triggered(bool checked);
    void onToolbarVisibilityChanged(bool visible);
    void on_menuExternal_aboutToShow();
    void on_actionUpgrade_triggered();
    void on_actionOpenXML_triggered();
    void onAutosaveTimeout();
    void on_actionGammaSRGB_triggered(bool checked);
    void on_actionGammaRec709_triggered(bool checked);
    void onFocusChanged(QWidget *old, QWidget * now) const;
    void onFocusObjectChanged(QObject *obj) const;
    void onFocusWindowChanged(QWindow *window) const;
    void onTimelineClipSelected();
    void onAddAllToTimeline(Mlt::Playlist* playlist, bool skipProxy);
    void on_actionScrubAudio_triggered(bool checked);
#if !defined(Q_OS_MAC)
    void onDrawingMethodTriggered(QAction*);
#endif
    void on_actionApplicationLog_triggered();
    void on_actionClose_triggered();
    void onPlayerTabIndexChanged(int index);
    void onUpgradeCheckFinished(QNetworkReply* reply);
    void onUpgradeTriggered();
    void onTimelineSelectionChanged();
    void on_actionCut_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void onClipCopied();
    void on_actionExportEDL_triggered();
    void on_actionExportFrame_triggered();
    void onGLWidgetImageReady();
    void on_actionAppDataSet_triggered();
    void on_actionAppDataShow_triggered();
    void on_actionNew_triggered();
    void on_actionKeyboardShortcuts_triggered();
    void on_actionLayoutLogging_triggered();
    void on_actionLayoutEditing_triggered();
    void on_actionLayoutEffects_triggered();
    void on_actionLayoutColor_triggered();
    void on_actionLayoutAudio_triggered();
    void on_actionLayoutPlayer_triggered();
    void on_actionLayoutPlaylist_triggered();
    void on_actionLayoutClip_triggered();
    void on_actionLayoutAdd_triggered();
    void onLayoutTriggered(QAction*);
    void on_actionProfileRemove_triggered();
    void on_actionLayoutRemove_triggered();
    void on_actionOpenOther2_triggered();
    void onOpenOtherTriggered(QWidget* widget);
    void onOpenOtherTriggered();
    void on_actionClearRecentOnExit_toggled(bool arg1);
    void onSceneGraphInitialized();
    void on_actionShowTextUnderIcons_toggled(bool b);
    void on_actionShowSmallIcons_toggled(bool b);
    void onPlaylistInChanged(int in);
    void onPlaylistOutChanged(int out);
    void on_actionPreviewNone_triggered(bool checked);
    void on_actionPreview360_triggered(bool checked);
    void on_actionPreview540_triggered(bool checked);
    void on_actionPreview720_triggered(bool checked);
    void on_actionTopics_triggered();
    void on_actionSync_triggered();
    void on_actionUseProxy_triggered(bool checked);
    void on_actionProxyStorageSet_triggered();
    void on_actionProxyStorageShow_triggered();
    void on_actionProxyUseProjectFolder_triggered(bool checked);
    void on_actionProxyUseHardware_triggered(bool checked);
    void on_actionProxyConfigureHardware_triggered();
    void updateLayoutSwitcher();
    void clearCurrentLayout();
};

#define MAIN MainWindow::singleton()

#endif // MAINWINDOW_H
