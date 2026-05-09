/*
 * Copyright (c) 2026 Meltytech, LLC
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

#include "actions.h"
#include "mainwindow.h"
#include "mltcontroller.h"

#include <ConsoleAppender.h>
#include <Logger.h>

#include <QFileInfo>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QUndoStack>

class TestMainWindow : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);

        // Mirror the setup done by Application in main.cpp
        QCoreApplication::setOrganizationName("Meltytech");
        QCoreApplication::setOrganizationDomain("shotcut.org");
        QCoreApplication::setApplicationName("Shotcut");
        QCoreApplication::setApplicationVersion(SHOTCUT_VERSION);

        // Suppress CuteLogger output: register an appender at Fatal level only
        // so the "no appenders" fallback to stderr is never triggered.
        auto *appender = new ConsoleAppender;
        appender->setDetailsLevel(Logger::Fatal);
        cuteLogger->registerAppender(appender);
    }

    // ── LayoutMode enum ──────────────────────────────────────────────────────
    void test_layoutModeEnumValues()
    {
        QCOMPARE(static_cast<int>(MainWindow::Custom), 0);
        QCOMPARE(static_cast<int>(MainWindow::Logging), 1);
        QCOMPARE(static_cast<int>(MainWindow::Editing), 2);
        QCOMPARE(static_cast<int>(MainWindow::Effects), 3);
        QCOMPARE(static_cast<int>(MainWindow::Color), 4);
        QCOMPARE(static_cast<int>(MainWindow::Audio), 5);
        QCOMPARE(static_cast<int>(MainWindow::PlayerOnly), 6);
    }

    // ── Singleton ────────────────────────────────────────────────────────────
    void test_singletonReturnsSameInstance()
    {
        MainWindow &w1 = MAIN;
        MainWindow &w2 = MAIN;
        QVERIFY(&w1 == &w2);
    }

    // ── Initial file / project state ─────────────────────────────────────────
    void test_initialFileNameIsEmpty()
    {
        QVERIFY(MAIN.fileName().isEmpty());
    }

    void test_initialMultitrackNotValid()
    {
        QVERIFY(!MAIN.isMultitrackValid());
    }

    void test_initialPlaylistNotValid()
    {
        QVERIFY(!MAIN.isPlaylistValid());
    }

    void test_initialClipboardNotNewer()
    {
        QVERIFY(!MAIN.isClipboardNewer());
    }

    // ── Undo stack ───────────────────────────────────────────────────────────
    void test_undoStackNotNull()
    {
        QVERIFY(MAIN.undoStack() != nullptr);
    }

    void test_undoStackInitiallyClean()
    {
        QVERIFY(MAIN.undoStack()->isClean());
    }

    void test_undoStackCannotUndoInitially()
    {
        QVERIFY(!MAIN.undoStack()->canUndo());
    }

    void test_undoStackCannotRedoInitially()
    {
        QVERIFY(!MAIN.undoStack()->canRedo());
    }

    // ── Dock and controller pointers ─────────────────────────────────────────
    void test_playlistDockNotNull()
    {
        QVERIFY(MAIN.playlistDock() != nullptr);
    }

    void test_timelineDockNotNull()
    {
        QVERIFY(MAIN.timelineDock() != nullptr);
    }

    void test_filterControllerNotNull()
    {
        QVERIFY(MAIN.filterController() != nullptr);
    }

    void test_customProfileMenuNotNull()
    {
        QVERIFY(MAIN.customProfileMenu() != nullptr);
    }

    void test_actionAddCustomProfileNotNull()
    {
        QVERIFY(MAIN.actionAddCustomProfile() != nullptr);
    }

    void test_actionProfileRemoveNotNull()
    {
        QVERIFY(MAIN.actionProfileRemove() != nullptr);
    }

    void test_profileGroupNotNull()
    {
        QVERIFY(MAIN.profileGroup() != nullptr);
    }

    // ── Window properties ────────────────────────────────────────────────────
    void test_windowAcceptDrops()
    {
        QVERIFY(MAIN.acceptDrops());
    }

    void test_windowNotModifiedInitially()
    {
        QVERIFY(!MAIN.isWindowModified());
    }

    void test_windowDockNestingEnabled()
    {
        QVERIFY(MAIN.isDockNestingEnabled());
    }

    void test_windowTitleContainsApplicationName()
    {
        QVERIFY(MAIN.windowTitle().contains(QCoreApplication::applicationName()));
    }

    // ── File path helpers ─────────────────────────────────────────────────────
    void test_untitledFileNameEndsWithUntitledMlt()
    {
        QVERIFY(MAIN.untitledFileName().endsWith("__untitled__.mlt"));
    }

    void test_untitledFileNameIsAbsolutePath()
    {
        QVERIFY(QFileInfo(MAIN.untitledFileName()).isAbsolute());
    }

    // ── Dock visibility ───────────────────────────────────────────────────────
    void test_keyframesDockNotVisibleInitially()
    {
        QVERIFY(!MAIN.keyframesDockIsVisible());
    }

    // ── Player actions registered ─────────────────────────────────────────────
    void test_actionPlayerPlayPauseRegistered()
    {
        QVERIFY(Actions["playerPlayPauseAction"] != nullptr);
    }

    void test_actionPlayerLoopRegistered()
    {
        QVERIFY(Actions["playerLoopAction"] != nullptr);
    }

    void test_actionPlayerFastForwardRegistered()
    {
        QVERIFY(Actions["playerFastForwardAction"] != nullptr);
    }

    void test_actionPlayerRewindRegistered()
    {
        QVERIFY(Actions["playerRewindAction"] != nullptr);
    }

    void test_actionPlayerSkipNextRegistered()
    {
        QVERIFY(Actions["playerSkipNextAction"] != nullptr);
    }

    void test_actionPlayerSkipPreviousRegistered()
    {
        QVERIFY(Actions["playerSkipPreviousAction"] != nullptr);
    }

    void test_actionPlayerSeekStartRegistered()
    {
        QVERIFY(Actions["playerSeekStartAction"] != nullptr);
    }

    void test_actionPlayerSeekEndRegistered()
    {
        QVERIFY(Actions["playerSeekEndAction"] != nullptr);
    }

    void test_actionPlayerNextFrameRegistered()
    {
        QVERIFY(Actions["playerNextFrameAction"] != nullptr);
    }

    void test_actionPlayerPreviousFrameRegistered()
    {
        QVERIFY(Actions["playerPreviousFrameAction"] != nullptr);
    }

    // ── MainWindow actions registered ─────────────────────────────────────────
    void test_actionTimelineReloadRegistered()
    {
        QVERIFY(Actions["timelineReload"] != nullptr);
    }

    void test_actionPropertiesRenameClipRegistered()
    {
        QVERIFY(Actions["propertiesRenameClipAction"] != nullptr);
    }

    void test_actionRecentFindRegistered()
    {
        QVERIFY(Actions["recentFindAction"] != nullptr);
    }

    void test_actionAnalyzeFiltersRegistered()
    {
        QVERIFY(Actions["analyzeFilters"] != nullptr);
    }

    // ── Timeline dock actions registered ─────────────────────────────────────
    void test_actionTimelineAddAudioTrackRegistered()
    {
        QVERIFY(Actions["timelineAddAudioTrackAction"] != nullptr);
    }

    void test_actionTimelineAddVideoTrackRegistered()
    {
        QVERIFY(Actions["timelineAddVideoTrackAction"] != nullptr);
    }

    void test_actionTimelineInsertTrackRegistered()
    {
        QVERIFY(Actions["timelineInsertTrackAction"] != nullptr);
    }

    void test_actionTimelineSelectAllRegistered()
    {
        QVERIFY(Actions["timelineSelectAllAction"] != nullptr);
    }

    void test_actionTimelineSelectNoneRegistered()
    {
        QVERIFY(Actions["timelineSelectNoneAction"] != nullptr);
    }

    void test_actionTimelineToggleTrackLockedRegistered()
    {
        QVERIFY(Actions["timelineToggleTrackLockedAction"] != nullptr);
    }

    void test_actionTimelineToggleTrackMuteRegistered()
    {
        QVERIFY(Actions["timelineToggleTrackMuteAction"] != nullptr);
    }

    // ── Filters dock actions registered ──────────────────────────────────────
    void test_actionFiltersAddFilterRegistered()
    {
        QVERIFY(Actions["filtersAddFilterAction"] != nullptr);
    }

    void test_actionFiltersRemoveFilterRegistered()
    {
        QVERIFY(Actions["filtersRemoveFilterAction"] != nullptr);
    }

    void test_actionFiltersCopyFiltersRegistered()
    {
        QVERIFY(Actions["filtersCopyFiltersAction"] != nullptr);
    }

    void test_actionFiltersPasteFiltersRegistered()
    {
        QVERIFY(Actions["filtersPasteFiltersAction"] != nullptr);
    }

    // ── Files and playlist dock actions registered ────────────────────────────
    void test_actionFilesSearchRegistered()
    {
        QVERIFY(Actions["filesSearch"] != nullptr);
    }

    void test_actionPlaylistSearchRegistered()
    {
        QVERIFY(Actions["playlistSearch"] != nullptr);
    }

    // ── MLT profile ───────────────────────────────────────────────────────────
    void test_mltProfileWidthPositive()
    {
        QVERIFY(MLT.profile().width() > 0);
    }

    void test_mltProfileHeightPositive()
    {
        QVERIFY(MLT.profile().height() > 0);
    }

    void test_mltProfileFpsPositive()
    {
        QVERIFY(MLT.profile().fps() > 0.0);
    }

    // ── Timeline empty-state helpers ──────────────────────────────────────────
    void test_bottomVideoTrackIndexNegativeWithEmptyTimeline()
    {
        QCOMPARE(MAIN.bottomVideoTrackIndex(), -1);
    }

    void test_mltIndexForTrackNegativeWithEmptyTimeline()
    {
        QCOMPARE(MAIN.mltIndexForTrack(0), -1);
    }

    // ── isSourceClipMyProject ─────────────────────────────────────────────────
    void test_isSourceClipMyProjectReturnsFalseForEmptyPath()
    {
        QVERIFY(!MAIN.isSourceClipMyProject("", false));
    }

    void test_isSourceClipMyProjectReturnsFalseForNonMatchingPath()
    {
        QVERIFY(!MAIN.isSourceClipMyProject("/nonexistent/file.mp4", false));
    }

    // ── Signal emission ───────────────────────────────────────────────────────
    void test_setProfileEmitsProfileChanged()
    {
        QSignalSpy spy(&MAIN, &MainWindow::profileChanged);
        MAIN.setProfile("dv_pal");
        QCOMPARE(spy.count(), 1);
        MAIN.setProfile(""); // restore to automatic
    }
};

QTEST_MAIN(TestMainWindow)
#include "mainwindowtest.moc"
