/*
 * Copyright (c) 2013-2022 Meltytech, LLC
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

#include "timelinedock.h"
#include "models/audiolevelstask.h"
#include "models/multitrackmodel.h"
#include "qmltypes/thumbnailprovider.h"
#include "mainwindow.h"
#include "commands/timelinecommands.h"
#include "qmltypes/qmlapplication.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlview.h"
#include "shotcut_mlt_properties.h"
#include "settings.h"
#include "util.h"
#include "proxymanager.h"
#include "dialogs/alignaudiodialog.h"
#include "dialogs/editmarkerdialog.h"
#include "dialogs/longuitask.h"
#include "widgets/docktoolbar.h"

#include <QAction>
#include <QMenu>
#include <QSlider>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtQml>
#include <QtQuick>
#include <QGuiApplication>
#include <QClipboard>
#include <Logger.h>

static const char *kFileUrlProtocol = "file://";
static const char *kFilesUrlDelimiter = ",file://";
static const int kRecordingTimerIntervalMs = 1000;

TimelineDock::TimelineDock(QWidget *parent) :
    QDockWidget(parent),
    m_quickView(QmlUtilities::sharedEngine(), this),
    m_position(-1),
    m_ignoreNextPositionChange(false),
    m_trimDelta(0),
    m_transitionDelta(0),
    m_blockSetSelection(false)
{
    LOG_DEBUG() << "begin";
    m_selection.selectedTrack = -1;
    m_selection.isMultitrackSelected = false;

    setObjectName("TimelineDock");
    QDockWidget::setWindowTitle(tr("Timeline"));
    resize(400, 300);
    setMinimumSize(QSize(200, 200));
    setAcceptDrops(true);
    QIcon icon = QIcon::fromTheme("view-time-schedule",
                                  QIcon(":/icons/oxygen/32x32/actions/view-time-schedule.png"));
    setWindowIcon(icon);
    setFeatures(QDockWidget::AllDockWidgetFeatures);
    setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    toggleViewAction()->setIcon(windowIcon());
    setupActions();

    m_mainMenu = new QMenu(this);
    QMenu *trackOperationsMenu = new QMenu(tr("Track Operations"), this);
    trackOperationsMenu->addAction(m_actions["timelineAddAudioTrackAction"]);
    trackOperationsMenu->addAction(m_actions["timelineAddVideoTrackAction"]);
    trackOperationsMenu->addAction(m_actions["timelineInsertTrackAction"]);
    trackOperationsMenu->addAction(m_actions["timelineRemoveTrackAction"]);
    trackOperationsMenu->addAction(m_actions["timelineMoveTrackUpAction"]);
    trackOperationsMenu->addAction(m_actions["timelineMoveTrackDownAction"]);
    trackOperationsMenu->addAction(m_actions["timelineToggleTrackHiddenAction"]);
    trackOperationsMenu->addAction(m_actions["timelineToggleTrackLockedAction"]);
    trackOperationsMenu->addAction(m_actions["timelineToggleTrackMuteAction"]);
    trackOperationsMenu->addAction(m_actions["timelineToggleTrackBlendingAction"]);
    m_mainMenu->addMenu(trackOperationsMenu);
    QMenu *trackHeightMenu = new QMenu(tr("Track Height"), this);
    trackHeightMenu->addAction(m_actions["timelineTracksShorterAction"]);
    trackHeightMenu->addAction(m_actions["timelineTracksTallerAction"]);
    trackHeightMenu->addAction(m_actions["timelineResetTrackHeightAction"]);
    m_mainMenu->addMenu(trackHeightMenu);
    QMenu *selectionMenu = new QMenu(tr("Selection"), this);
    selectionMenu->addAction(m_actions["timelineSelectAllAction"]);
    selectionMenu->addAction(m_actions["timelineSelectAllOnTrackAction"]);
    selectionMenu->addAction(m_actions["timelineSelectNoneAction"]);
    selectionMenu->addAction(m_actions["timelineSelectNextClipAction"]);
    selectionMenu->addAction(m_actions["timelineSelectPrevClipAction"]);
    selectionMenu->addAction(m_actions["timelineSelectClipAboveAction"]);
    selectionMenu->addAction(m_actions["timelineSelectClipBelowAction"]);
    selectionMenu->addAction(m_actions["timelineSelectTrackAboveAction"]);
    selectionMenu->addAction(m_actions["timelineSelectTrackBelowAction"]);
    selectionMenu->addAction(m_actions["timelineSelectClipUnderPlayheadAction"]);
    m_mainMenu->addMenu(selectionMenu);
    QMenu *editMenu = new QMenu(tr("Edit"), this);
    editMenu->addAction(m_actions["timelineCutAction"]);
    editMenu->addAction(m_actions["timelineCopyAction"]);
    editMenu->addAction(m_actions["timelinePasteAction"]);
    editMenu->addAction(m_actions["timelineAppendAction"]);
    editMenu->addAction(m_actions["timelineDeleteAction"]);
    editMenu->addAction(m_actions["timelineLiftAction"]);
    editMenu->addAction(m_actions["timelineOverwriteAction"]);
    editMenu->addAction(m_actions["timelineSplitAction"]);
    editMenu->addAction(m_actions["timelineReplaceAction"]);
    m_mainMenu->addMenu(editMenu);
    QMenu *viewMenu = new QMenu(tr("View"), this);
    viewMenu->addAction(m_actions["timelineZoomOutAction"]);
    viewMenu->addAction(m_actions["timelineZoomInAction"]);
    viewMenu->addAction(m_actions["timelineZoomFitAction"]);
    viewMenu->addAction(m_actions["timelinePropertiesAction"]);
    m_mainMenu->addMenu(viewMenu);
    QMenu *markerMenu = new QMenu(tr("Marker"), this);
    markerMenu->addAction(m_actions["timelineMarkerAction"]);
    markerMenu->addAction(m_actions["timelinePrevMarkerAction"]);
    markerMenu->addAction(m_actions["timelineNextMarkerAction"]);
    markerMenu->addAction(m_actions["timelineDeleteMarkerAction"]);
    markerMenu->addAction(m_actions["timelineMarkSelectedClipAction"]);
    m_mainMenu->addMenu(markerMenu);
    m_mainMenu->addAction(m_actions["timelineRecordAudioAction"]);

    m_clipMenu = new QMenu(this);
    m_clipMenu->addAction(m_actions["timelineCutAction"]);
    m_clipMenu->addAction(m_actions["timelineCopyAction"]);
    m_clipMenu->addAction(m_actions["timelineDeleteAction"]);
    m_clipMenu->addAction(m_actions["timelineLiftAction"]);
    m_clipMenu->addAction(m_actions["timelineReplaceAction"]);
    QMenu *clipMoreMenu = new QMenu(tr("More"), this);
    clipMoreMenu->addAction(m_actions["timelineMergeWithNextAction"]);
    clipMoreMenu->addAction(m_actions["timelineDetachAudioAction"]);
    clipMoreMenu->addAction(m_actions["timelineAlignToReferenceAction"]);
    clipMoreMenu->addAction(m_actions["timelineUpdateThumbnailsAction"]);
    clipMoreMenu->addAction(m_actions["timelineRebuildAudioWaveformAction"]);
    m_clipMenu->addMenu(clipMoreMenu);
    m_clipMenu->addAction(m_actions["timelinePropertiesAction"]);

    QVBoxLayout *vboxLayout = new QVBoxLayout();
    vboxLayout->setSpacing(0);
    vboxLayout->setContentsMargins(0, 0, 0, 0);

    DockToolBar *toolbar = new DockToolBar(tr("Timeline Controls"));
    QToolButton *menuButton = new QToolButton();
    menuButton->setIcon(QIcon::fromTheme("show-menu",
                                         QIcon(":/icons/oxygen/32x32/actions/show-menu.png")));
    menuButton->setToolTip(tr("Timeline Menu"));
    menuButton->setAutoRaise(true);
    menuButton->setPopupMode(QToolButton::QToolButton::InstantPopup);
    menuButton->setMenu(m_mainMenu);
    toolbar->addWidget(menuButton);
    toolbar->addSeparator();
    toolbar->addAction(m_actions["timelineCutAction"]);
    toolbar->addAction(m_actions["timelineCopyAction"]);
    toolbar->addAction(m_actions["timelinePasteAction"]);
    toolbar->addSeparator();
    toolbar->addAction(m_actions["timelineAppendAction"]);
    toolbar->addAction(m_actions["timelineDeleteAction"]);
    toolbar->addAction(m_actions["timelineLiftAction"]);
    toolbar->addAction(m_actions["timelineOverwriteAction"]);
    toolbar->addAction(m_actions["timelineSplitAction"]);
    toolbar->addSeparator();
    toolbar->addAction(m_actions["timelineMarkerAction"]);
    toolbar->addAction(m_actions["timelinePrevMarkerAction"]);
    toolbar->addAction(m_actions["timelineNextMarkerAction"]);
    toolbar->addSeparator();
    toolbar->addAction(m_actions["timelineSnapAction"]);
    toolbar->addAction(m_actions["timelineScrubDragAction"]);
    toolbar->addAction(m_actions["timelineRippleAction"]);
    toolbar->addAction(m_actions["timelineRippleAllTracksAction"]);
    toolbar->addAction(m_actions["timelineRippleMarkersAction"]);
    toolbar->addSeparator();
    toolbar->addAction(m_actions["timelineZoomOutAction"]);
    QSlider *zoomSlider = new QSlider();
    zoomSlider->setOrientation(Qt::Horizontal);
    zoomSlider->setMaximumWidth(200);
    zoomSlider->setMinimum(0);
    zoomSlider->setMaximum(300);
    zoomSlider->setValue(100);
    connect(zoomSlider, &QSlider::valueChanged, this, [&](int value) {
        if (!isVisible() || !m_quickView.rootObject()) return;
        setZoom(value / 100.0);
    });
    connect(&m_model, &MultitrackModel::scaleFactorChanged, zoomSlider, [ = ]() {
        double value = round(pow(m_model.scaleFactor() - 0.01, 1.0 / 3.0) * 100.0);
        zoomSlider->setValue(value);
    });
    toolbar->addWidget(zoomSlider);
    toolbar->addAction(m_actions["timelineZoomInAction"]);
    toolbar->addAction(m_actions["timelineZoomFitAction"]);
    toolbar->addSeparator();
    toolbar->addAction(m_actions["timelineRecordAudioAction"]);
    vboxLayout->setMenuBar(toolbar);

    qmlRegisterType<MultitrackModel>("Shotcut.Models", 1, 0, "MultitrackModel");
    qmlRegisterType<MarkersModel>("Shotcut.Models", 1, 0, "MarkersModel");

    QDir importPath = QmlUtilities::qmlDir();
    importPath.cd("modules");
    m_quickView.engine()->addImportPath(importPath.path());
    m_quickView.engine()->addImageProvider(QString("thumbnail"), new ThumbnailProvider);
    QmlUtilities::setCommonProperties(m_quickView.rootContext());
    m_quickView.rootContext()->setContextProperty("view", new QmlView(&m_quickView));
    m_quickView.rootContext()->setContextProperty("timeline", this);
    m_quickView.rootContext()->setContextProperty("multitrack", &m_model);
    m_quickView.rootContext()->setContextProperty("markers", &m_markersModel);
    m_quickView.setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickView.setClearColor(palette().window().color());
    m_quickView.quickWindow()->setPersistentSceneGraph(false);
#ifndef Q_OS_MAC
    m_quickView.setAttribute(Qt::WA_AcceptTouchEvents);
#endif

    connect(&m_model, SIGNAL(modified()), this, SLOT(clearSelectionIfInvalid()));
    connect(&m_model, &MultitrackModel::appended, this, &TimelineDock::selectClip,
            Qt::QueuedConnection);
    connect(&m_model, &MultitrackModel::inserted, this, &TimelineDock::selectClip,
            Qt::QueuedConnection);
    connect(&m_model, &MultitrackModel::overWritten, this, &TimelineDock::selectClip,
            Qt::QueuedConnection);
    connect(&m_model, SIGNAL(rowsInserted(QModelIndex, int, int)), SLOT(onRowsInserted(QModelIndex, int,
                                                                                       int)));
    connect(&m_model, SIGNAL(rowsRemoved(QModelIndex, int, int)), SLOT(onRowsRemoved(QModelIndex, int,
                                                                                     int)));
    connect(&m_model, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
            SLOT(onRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
    connect(&m_model, SIGNAL(closed()), SLOT(onMultitrackClosed()));
    connect(&m_model, SIGNAL(created()), SLOT(reloadTimelineMarkers()));
    connect(&m_model, SIGNAL(loaded()), SLOT(reloadTimelineMarkers()));
    connect(&m_model, SIGNAL(closed()), SLOT(reloadTimelineMarkers()));

    vboxLayout->addWidget(&m_quickView);

    connect(this, SIGNAL(clipMoved(int, int, int, int, bool)), SLOT(onClipMoved(int, int, int, int,
                                                                                bool)), Qt::QueuedConnection);
    connect(this, SIGNAL(transitionAdded(int, int, int, bool)), SLOT(onTransitionAdded(int, int, int,
                                                                                       bool)), Qt::QueuedConnection);
    connect(MLT.videoWidget(), SIGNAL(frameDisplayed(const SharedFrame &)), this,
            SLOT(onShowFrame(const SharedFrame &)));
    connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(load()));
    connect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(onTopLevelChanged(bool)));
    connect(this, SIGNAL(warnTrackLocked(int)), SLOT(onWarnTrackLocked()));
    connect(&m_markersModel, SIGNAL(rangesChanged()), this, SIGNAL(markerRangesChanged()));

    QWidget *dockContentsWidget = new QWidget();
    dockContentsWidget->setLayout(vboxLayout);
    QDockWidget::setWidget(dockContentsWidget);
    LOG_DEBUG() << "end";
}

TimelineDock::~TimelineDock()
{
}

void TimelineDock::setupActions()
{
    QIcon icon;
    QAction *action;

    action = new QAction(tr("Add Audio Track"), this);
    action->setObjectName("timelineAddAudioTrackAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
    connect(action, &QAction::triggered, this, [&]() {
        show();
        raise();
        addAudioTrack();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Add Video Track"), this);
    action->setObjectName("timelineAddVideoTrackAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));
    connect(action, &QAction::triggered, this, [&]() {
        show();
        raise();
        addVideoTrack();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Insert Track"), this);
    action->setObjectName("timelineInsertTrackAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_I));
    connect(action, &QAction::triggered, this, [&]() {
        show();
        raise();
        insertTrack();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Remove Track"), this);
    action->setObjectName("timelineRemoveTrackAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_U));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        removeTrack();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Move Track Up"), this);
    action->setObjectName("timelineMoveTrackUpAction");
    action->setShortcut(QKeySequence(Qt::SHIFT + Qt::ALT + Qt::Key_Up));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        moveTrackUp();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Move Track Down"), this);
    action->setObjectName("timelineMoveTrackDownAction");
    action->setShortcut(QKeySequence(Qt::SHIFT + Qt::ALT + Qt::Key_Down));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        moveTrackDown();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Show/Hide Selected Track"), this);
    action->setObjectName("timelineToggleTrackHiddenAction");
#ifdef Q_OS_MAC
    // OS X uses Cmd+H to hide an app.
    action->setShortcut(QKeySequence(Qt::META + Qt::Key_H));
#else
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
#endif
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        toggleTrackHidden(currentTrack());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Lock/Unlock Selected Track"), this);
    action->setObjectName("timelineToggleTrackLockedAction");
#ifdef Q_OS_MAC
    // OS X uses Cmd+H to hide an app and Cmd+M to minimize. Therefore, we force
    // it to be the apple keyboard control key aka meta. Therefore, to be
    // consistent with all track header toggles, we make the lock toggle also use
    // meta.
    action->setShortcut(QKeySequence(Qt::META + Qt::Key_L));
#else
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
#endif
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        setTrackLock(currentTrack(), !isTrackLocked(currentTrack()));
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Mute/Unmute Selected Track"), this);
    action->setObjectName("timelineToggleTrackMuteAction");
#ifdef Q_OS_MAC
    // OS X uses Cmd+M to minimize an app.
    action->setShortcut(QKeySequence(Qt::META + Qt::Key_M));
#else
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
#endif
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        toggleTrackMute(currentTrack());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Blend/Unblend Selected Track"), this);
    action->setObjectName("timelineToggleTrackBlendingAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_B));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        int trackIndex = currentTrack();
        if (trackIndex != model()->bottomVideoTrackIndex()) {
            bool isComposite = model()->data(model()->index(trackIndex),
                                             MultitrackModel::IsCompositeRole).toBool();
            setTrackComposite(trackIndex, !isComposite);
        }
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Make Tracks Shorter"), this);
    action->setObjectName("timelineTracksShorterAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid() || !isVisible()) return;
        m_model.setTrackHeight(std::max(10, m_model.trackHeight() - 20));
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Make Tracks Taller"), this);
    action->setObjectName("timelineTracksTallerAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid() || !isVisible()) return;
        m_model.setTrackHeight(m_model.trackHeight() + 20);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Reset Track Height"), this);
    action->setObjectName("timelineResetTrackHeightAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Equal));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid() || !isVisible()) return;
        m_model.setTrackHeight(50);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select All"), this);
    action->setObjectName("timelineSelectAllAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        selectAll();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select All On Current Track"), this);
    action->setObjectName("timelineSelectAllOnTrackAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_A));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        selectAllOnCurrentTrack();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select None"), this);
    action->setObjectName("timelineSelectNoneAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        setSelection();
        model()->reload();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Next Clip"), this);
    action->setObjectName("timelineSelectNextClipAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid() || !isVisible()) return;
        if (selection().isEmpty()) {
            selectClipUnderPlayhead();
        } else if (selection().size() == 1) {
            int newIndex = selection().first().x() + 1;
            if (newIndex < clipCount(-1))
                setSelection(QList<QPoint>() << QPoint(newIndex, selection().first().y()));
        }
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Previous Clip"), this);
    action->setObjectName("timelineSelectPrevClipAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid() || !isVisible()) return;
        if (selection().isEmpty()) {
            selectClipUnderPlayhead();
        } else if (selection().size() == 1) {
            int newIndex = selection().first().x() - 1;
            if (newIndex >= 0)
                setSelection(QList<QPoint>() << QPoint(newIndex, selection().first().y()));
        }
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Clip Above"), this);
    action->setObjectName("timelineSelectClipAboveAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid() || !isVisible()) return;
        int newClipIndex = -1;
        int trackIndex = currentTrack() - 1;
        if (!selection().isEmpty() && trackIndex > -1) {
            int navigationPosition = centerOfClip(selection().first().y(),
                                                  selection().first().x());
            newClipIndex = clipIndexAtPosition(trackIndex, navigationPosition);
        }
        incrementCurrentTrack(-1);
        if (newClipIndex >= 0) {
            newClipIndex = qMin(newClipIndex, clipCount(trackIndex) - 1);
            setSelection(QList<QPoint>() << QPoint(newClipIndex, trackIndex));
        }
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Clip Below"), this);
    action->setObjectName("timelineSelectClipBelowAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid() || !isVisible()) return;
        int newClipIndex = -1;
        int trackIndex = currentTrack() + 1;
        if (!selection().isEmpty() && trackIndex < model()->trackList().count()) {
            int navigationPosition = centerOfClip(selection().first().y(),
                                                  selection().first().x());
            newClipIndex = clipIndexAtPosition(trackIndex, navigationPosition);
        }
        incrementCurrentTrack(1);
        if (newClipIndex >= 0) {
            newClipIndex = qMin(newClipIndex, clipCount(trackIndex) - 1);
            setSelection(QList<QPoint>() << QPoint(newClipIndex, trackIndex));
        }
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Track Above"), this);
    action->setObjectName("timelineSelectTrackAboveAction");
    action->setShortcut(QKeySequence(Qt::Key_Up));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid() || !isVisible()) return;
        incrementCurrentTrack(-1);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Track Below"), this);
    action->setObjectName("timelineSelectTrackBelowAction");
    action->setShortcut(QKeySequence(Qt::Key_Down));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid() || !isVisible()) return;
        incrementCurrentTrack(1);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Clip Under Playhead"), this);
    action->setObjectName("timelineSelectClipUnderPlayheadAction");
    QList<QKeySequence> clipUnderPlayheadShortcuts;
    clipUnderPlayheadShortcuts << QKeySequence(Qt::CTRL + Qt::Key_Space);
#ifdef Q_OS_MAC
    // Spotlight defaults to Cmd+Space, so also accept Ctrl+Space.
    clipUnderPlayheadShortcuts << QKeySequence(Qt::META + Qt::Key_Space);
#endif
    action->setShortcuts(clipUnderPlayheadShortcuts);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid() || !isVisible()) return;
        selectClipUnderPlayhead();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Cu&t"), this);
    action->setObjectName("timelineCutAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X));
    icon = QIcon::fromTheme("edit-cut",
                            QIcon(":/icons/oxygen/32x32/actions/edit-cut.png"));
    action->setIcon(icon);
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [&]() {
        show();
        raise();
        removeSelection(true);
    });
    connect(this, &TimelineDock::selectionChanged, action, [ = ]() {
        bool enabled = m_selection.selectedClips.length() > 0;
        if (enabled) {
            int trackIndex = selection().first().y();
            int clipIndex = selection().first().x();
            enabled = !isBlank(trackIndex, clipIndex) && !isTransition(trackIndex, clipIndex);
        }
        action->setEnabled(enabled);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("&Copy"), this);
    action->setObjectName("timelineCopyAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    icon = QIcon::fromTheme("edit-copy",
                            QIcon(":/icons/oxygen/32x32/actions/edit-copy.png"));
    action->setIcon(icon);
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [&]() {
        show();
        raise();
        if (selection().isEmpty()) {
            copy(-1, -1);
        } else {
            auto &selected = selection().first();
            copy(selected.y(), selected.x());
        }
    });
    connect(this, &TimelineDock::selectionChanged, action, [ = ]() {
        bool enabled = m_selection.selectedClips.length() > 0;
        if (enabled) {
            int trackIndex = selection().first().y();
            int clipIndex = selection().first().x();
            enabled = !isBlank(trackIndex, clipIndex) && !isTransition(trackIndex, clipIndex);
        }
        action->setEnabled(enabled);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("&Paste"), this);
    action->setObjectName("timelinePasteAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_V));
    icon = QIcon::fromTheme("edit-paste",
                            QIcon(":/icons/oxygen/32x32/actions/edit-paste.png"));
    action->setIcon(icon);
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [&]() {
        show();
        raise();
        insert(-1);
    });
    connect(this, &TimelineDock::clipCopied, action, [ = ]() {
        action->setEnabled(true);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Append"), this);
    action->setObjectName("timelineAppendAction");
    action->setShortcut(QKeySequence(Qt::Key_A));
    icon = QIcon::fromTheme("list-add",
                            QIcon(":/icons/oxygen/32x32/actions/list-add.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        show();
        raise();
        append(currentTrack());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Ripple Delete"), this);
    action->setObjectName("timelineDeleteAction");
    QList<QKeySequence> deleteShortcuts;
    deleteShortcuts << QKeySequence(Qt::Key_X);
    deleteShortcuts << QKeySequence(Qt::SHIFT + Qt::Key_Backspace);
    deleteShortcuts << QKeySequence(Qt::SHIFT + Qt::Key_Delete);
    action->setShortcuts(deleteShortcuts);
    icon = QIcon::fromTheme("list-remove",
                            QIcon(":/icons/oxygen/32x32/actions/list-remove.png"));
    action->setIcon(icon);
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        removeSelection();
    });
    connect(this, &TimelineDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_selection.selectedClips.length() > 0);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Lift"), this);
    action->setObjectName("timelineLiftAction");
    action->setShortcut(QKeySequence(Qt::Key_Z));
    QList<QKeySequence> liftShortcuts;
    liftShortcuts << QKeySequence(Qt::Key_Z);
    liftShortcuts << QKeySequence(Qt::Key_Backspace);
    liftShortcuts << QKeySequence(Qt::Key_Delete);
    action->setShortcuts(liftShortcuts);
    icon = QIcon::fromTheme("lift",
                            QIcon(":/icons/oxygen/32x32/actions/lift.png"));
    action->setIcon(icon);
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        liftSelection();
    });
    connect(this, &TimelineDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_selection.selectedClips.length() > 0);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Overwrite"), this);
    action->setObjectName("timelineOverwriteAction");
    action->setShortcut(QKeySequence(Qt::Key_B));
    icon = QIcon::fromTheme("overwrite",
                            QIcon(":/icons/oxygen/32x32/actions/overwrite.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        overwrite(currentTrack());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Split At Playhead"), this);
    action->setObjectName("timelineSplitAction");
    action->setShortcut(QKeySequence(Qt::Key_S));
    icon = QIcon::fromTheme("slice",
                            QIcon(":/icons/oxygen/32x32/actions/slice.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        splitClip(currentTrack());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Replace"), this);
    action->setObjectName("timelineReplaceAction");
    action->setShortcut(QKeySequence(Qt::Key_R));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        if (MLT.isClip() || selection().isEmpty()) {
            replace(-1, -1);
        } else {
            auto &selected = selection().first();
            replace(selected.y(), selected.x());
        }
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Create/Edit Marker"), this);
    action->setObjectName("timelineMarkerAction");
    action->setShortcut(QKeySequence(Qt::Key_M));
    icon = QIcon::fromTheme("marker",
                            QIcon(":/icons/oxygen/32x32/actions/marker.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        createOrEditMarker();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Previous Marker"), this);
    action->setObjectName("timelinePrevMarkerAction");
    action->setShortcut(QKeySequence(Qt::Key_Less));
    icon = QIcon::fromTheme("format-indent-less",
                            QIcon(":/icons/oxygen/32x32/actions/format-indent-less.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        seekPrevMarker();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Next Marker"), this);
    action->setObjectName("timelineNextMarkerAction");
    action->setShortcut(QKeySequence(Qt::Key_Greater));
    icon = QIcon::fromTheme("format-indent-more",
                            QIcon(":/icons/oxygen/32x32/actions/format-indent-more.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        seekNextMarker();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Delete Marker"), this);
    action->setObjectName("timelineDeleteMarkerAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        deleteMarker();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Create Marker Around Selected Clip"), this);
    action->setObjectName("timelineMarkSelectedClipAction");
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_M));
    connect(action, &QAction::triggered, this, [&]() {
        if (!isMultitrackValid()) return;
        show();
        raise();
        createOrEditSelectionMarker();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Snap"), this);
    action->setObjectName("timelineSnapAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
    icon = QIcon::fromTheme("snap",
                            QIcon(":/icons/oxygen/32x32/actions/snap.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    action->setChecked(Settings.timelineSnap());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setTimelineSnap(checked);
    });
    connect(&Settings, &ShotcutSettings::timelineSnapChanged, action, [ = ]() {
        action->setChecked(Settings.timelineSnap());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Scrub While Dragging"), this);
    action->setObjectName("timelineScrubDragAction");
    icon = QIcon::fromTheme("scrub_drag",
                            QIcon(":/icons/oxygen/32x32/actions/scrub_drag.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    action->setChecked(Settings.timelineDragScrub());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setTimelineDragScrub(checked);
    });
    connect(&Settings, &ShotcutSettings::timelineDragScrubChanged, action, [ = ]() {
        action->setChecked(Settings.timelineDragScrub());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Ripple"), this);
    action->setObjectName("timelineRippleAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    icon = QIcon::fromTheme("target",
                            QIcon(":/icons/oxygen/32x32/actions/target.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    action->setChecked(Settings.timelineRipple());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setTimelineRipple(checked);
    });
    connect(&Settings, &ShotcutSettings::timelineRippleChanged, action, [ = ]() {
        action->setChecked(Settings.timelineRipple());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Ripple All Tracks"), this);
    action->setObjectName("timelineRippleAllTracksAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_R));
    icon = QIcon::fromTheme("ripple-all",
                            QIcon(":/icons/oxygen/32x32/actions/ripple-all.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    action->setChecked(Settings.timelineRippleAllTracks());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setTimelineRippleAllTracks(checked);
    });
    connect(&Settings, &ShotcutSettings::timelineRippleAllTracksChanged, action, [ = ]() {
        action->setChecked(Settings.timelineRippleAllTracks());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Ripple Markers"), this);
    action->setObjectName("timelineRippleMarkersAction");
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_R));
    icon = QIcon::fromTheme("ripple-marker",
                            QIcon(":/icons/oxygen/32x32/actions/ripple-marker.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    action->setChecked(Settings.timelineRippleMarkers());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setTimelineRippleMarkers(checked);
    });
    connect(&Settings, &ShotcutSettings::timelineRippleMarkersChanged, action, [ = ]() {
        action->setChecked(Settings.timelineRippleMarkers());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Show Audio Waveforms"), this);
    action->setObjectName("timelineShowWaveformsAction");
    action->setCheckable(true);
    action->setChecked(Settings.timelineShowWaveforms());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setTimelineShowWaveforms(checked);
        if (!isVisible() || !m_quickView.rootObject()) return;
        refreshWaveforms();
    });
    connect(&Settings, &ShotcutSettings::timelineShowWaveformsChanged, action, [ = ]() {
        action->setChecked(Settings.timelineShowWaveforms());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Use Higher Performance Waveforms"), this);
    action->setObjectName("timelinePerformanceWaveformsAction");
    action->setCheckable(true);
    action->setChecked(Settings.timelineFramebufferWaveform());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setTimelineFramebufferWaveform(checked);
        if (!isVisible() || !m_quickView.rootObject()) return;
        if (Settings.timelineFramebufferWaveform()) {
            m_model.reload();
        }
    });
    connect(&Settings, &ShotcutSettings::timelineFramebufferWaveformChanged, action, [ = ]() {
        action->setChecked(Settings.timelineFramebufferWaveform());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Show Video Thumbnails"), this);
    action->setObjectName("timelineShowThumbnailsAction");
    action->setCheckable(true);
    action->setChecked(Settings.timelineShowThumbnails());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setTimelineShowThumbnails(checked);
    });
    connect(&Settings, &ShotcutSettings::timelineShowThumbnailsChanged, action, [ = ]() {
        action->setChecked(Settings.timelineShowThumbnails());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Center the Playhead"), this);
    action->setObjectName("timelineCenterPlayheadAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_P));
    action->setCheckable(true);
    action->setChecked(Settings.timelineCenterPlayhead());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setTimelineCenterPlayhead(checked);
    });
    connect(&Settings, &ShotcutSettings::timelineCenterPlayheadChanged, action, [ = ]() {
        action->setChecked(Settings.timelineCenterPlayhead());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Scroll to Playhead on Zoom"), this);
    action->setObjectName("timelineScrollZoomAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_P));
    action->setCheckable(true);
    action->setChecked(Settings.timelineScrollZoom());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setTimelineScrollZoom(checked);
    });
    connect(&Settings, &ShotcutSettings::timelineScrollZoomChanged, action, [ = ]() {
        action->setChecked(Settings.timelineScrollZoom());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Zoom Timeline Out"), this);
    action->setObjectName("timelineZoomOutAction");
    action->setShortcut(QKeySequence(Qt::Key_Minus));
    icon = QIcon::fromTheme("zoom-out",
                            QIcon(":/icons/oxygen/32x32/actions/zoom-out.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isVisible() || !m_quickView.rootObject()) return;
        zoomOut();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Zoom Timeline In"), this);
    action->setObjectName("timelineZoomInAction");
    action->setShortcut(QKeySequence(Qt::Key_Plus));
    icon = QIcon::fromTheme("zoom-in",
                            QIcon(":/icons/oxygen/32x32/actions/zoom-in.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isVisible() || !m_quickView.rootObject()) return;
        zoomIn();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Zoom Timeline To Fit"), this);
    action->setObjectName("timelineZoomFitAction");
    action->setShortcut(QKeySequence(Qt::Key_0));
    icon = QIcon::fromTheme("zoom-fit-best",
                            QIcon(":/icons/oxygen/32x32/actions/zoom-fit-best.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isVisible()) return;
        zoomToFit();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Record Audio"), this);
    action->setObjectName("timelineRecordAudioAction");
    icon = QIcon::fromTheme("audio-input-microphone",
                            QIcon(":/icons/oxygen/32x32/devices/audio-input-microphone.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    action->setChecked(isRecording());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        if (isRecording())
            stopRecording();
        else
            recordAudio();
    });
    connect(this, &TimelineDock::isRecordingChanged, action, [ = ]() {
        action->setChecked(isRecording());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Properties"), this);
    action->setObjectName("timelinePropertiesAction");
    connect(action, &QAction::triggered, this, [&](bool checked) {
        openProperties();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Merge With Next Clip"), this);
    action->setObjectName("timelineMergeWithNextAction");
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        if (m_selection.selectedClips.length() == 1) {
            mergeClipWithNext(selection().first().y(), selection().first().x(), false);
        }
    });
    connect(this, &TimelineDock::selectionChanged, action, [ = ]() {
        bool enabled = false;
        if (m_selection.selectedClips.length() == 1) {
            enabled = mergeClipWithNext(selection().first().y(), selection().first().x(), true);
        }
        action->setEnabled(enabled);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Detach Audio"), this);
    action->setObjectName("timelineDetachAudioAction");
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        if (m_selection.selectedClips.length() == 1) {
            detachAudio(selection().first().y(), selection().first().x());
        }
    });
    connect(this, &TimelineDock::selectionChanged, action, [ = ]() {
        bool enabled = false;
        if (m_selection.selectedClips.length() == 1) {
            int trackIndex = selection().first().y();
            int clipIndex = selection().first().x();
            if (trackIndex >= 0 && clipIndex >= 0) {
                QModelIndex modelIndex = m_model.index(clipIndex, 0, m_model.index(trackIndex));
                if (modelIndex.isValid()) {
                    enabled = !modelIndex.data(MultitrackModel::IsBlankRole).toBool()
                              && !modelIndex.data(MultitrackModel::IsTransitionRole).toBool()
                              && !modelIndex.data(MultitrackModel::IsAudioRole).toBool()
                              && (modelIndex.data(MultitrackModel::AudioIndexRole).toInt() > -1
                                  || modelIndex.data(MultitrackModel::AudioIndexRole).toString() == "all");
                }
            }
        }
        action->setEnabled(enabled);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Align To Reference Track"), this);
    action->setObjectName("timelineAlignToReferenceAction");
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        if (m_selection.selectedClips.length() > 0) {
            alignSelectedClips();
        }
    });
    connect(this, &TimelineDock::selectionChanged, action, [ = ]() {
        bool enabled = false;
        foreach (auto point, selection()) {
            // At least one selected item must be a valid clip
            if (!isBlank(point.y(), point.x()) && !isTransition(point.y(), point.x())) {
                enabled = true;
                break;
            }
        }
        action->setEnabled(enabled);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Update Thumbnails"), this);
    action->setObjectName("timelineUpdateThumbnailsAction");
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        foreach (auto point, selection()) {
            if (!isBlank(point.y(), point.x()) && !isTransition(point.y(), point.x())) {
                emit updateThumbnails(point.y(), point.x());
            }
        }
    });
    connect(this, &TimelineDock::selectionChanged, action, [ = ]() {
        bool enabled = false;
        if (Settings.timelineShowThumbnails()) {
            foreach (auto point, selection()) {
                // At least one selected item must be a valid clip
                if (!isBlank(point.y(), point.x()) && !isTransition(point.y(), point.x())) {
                    enabled = true;
                    break;
                }
            }
        }
        action->setEnabled(enabled);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Rebuild Audio Waveform"), this);
    action->setObjectName("timelineRebuildAudioWaveformAction");
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        foreach (auto point, selection()) {
            if (!isBlank(point.y(), point.x()) && !isTransition(point.y(), point.x())) {
                remakeAudioLevels(point.y(), point.x());
            }
        }
    });
    connect(this, &TimelineDock::selectionChanged, action, [ = ]() {
        bool enabled = false;
        if (Settings.timelineShowWaveforms()) {
            foreach (auto point, selection()) {
                // At least one selected item must be a valid clip
                if (!isBlank(point.y(), point.x()) && !isTransition(point.y(), point.x())) {
                    enabled = true;
                    break;
                }
            }
        }
        action->setEnabled(enabled);
    });
    m_actions[action->objectName()] = action;
}

void TimelineDock::setPosition(int position)
{
    if (!m_model.tractor()) return;
    if (position <= m_model.tractor()->get_length()) {
        emit seeked(position);
    } else {
        m_position = m_model.tractor()->get_length();
        emit positionChanged();
    }
}

Mlt::ClipInfo *TimelineDock::getClipInfo(int trackIndex, int clipIndex)
{
    Mlt::ClipInfo *result = nullptr;
    if (clipIndex >= 0 && trackIndex >= 0 && trackIndex < m_model.trackList().size()) {
        int i = m_model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            result = playlist.clip_info(clipIndex);
        }
    }
    return result;
}

Mlt::Producer TimelineDock::producerForClip(int trackIndex, int clipIndex)
{
    Mlt::Producer result;
    Mlt::ClipInfo *info = getClipInfo(trackIndex, clipIndex);
    if (info) {
        result = Mlt::Producer(info->producer);
        delete info;
    }
    return result;
}

int TimelineDock::clipIndexAtPlayhead(int trackIndex)
{
    return clipIndexAtPosition(trackIndex, m_position);
}

int TimelineDock::clipIndexAtPosition(int trackIndex, int position)
{
    int result = -1;
    if (trackIndex < 0)
        trackIndex = currentTrack();
    if (trackIndex >= 0 && trackIndex < m_model.trackList().size()) {
        int i = m_model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            result = playlist.get_clip_index_at(position);
            if (result >= playlist.count())
                result = -1;
        }
    }
    return result;
}

bool TimelineDock::isBlank(int trackIndex, int clipIndex)
{
    return trackIndex >= 0 && clipIndex >= 0 &&
           m_model.index(clipIndex, 0, m_model.index(trackIndex))
           .data(MultitrackModel::IsBlankRole).toBool();
}

bool TimelineDock::isTransition(int trackIndex, int clipIndex)
{
    return trackIndex >= 0 && clipIndex >= 0 &&
           m_model.index(clipIndex, 0, m_model.index(trackIndex))
           .data(MultitrackModel::IsTransitionRole).toBool();
}

void TimelineDock::onWarnTrackLocked()
{
    emit showStatusMessage(tr("This track is locked"));
}

void TimelineDock::emitNonSeekableWarning()
{
    emit showStatusMessage(tr("You cannot add a non-seekable source."));
}

void TimelineDock::addTrackIfNeeded(int trackIndex, Mlt::Producer *srcTrack)
{
    const auto n = m_model.trackList().size();
    if (trackIndex >= n) {
        if (m_selection.selectedTrack != -1)
            setSelection();
        if (srcTrack->get_int(kAudioTrackProperty) || (n > 0
                                                       && m_model.trackList()[n - 1].type == AudioTrackType)) {
            MAIN.undoStack()->push(
                new Timeline::InsertTrackCommand(m_model, trackIndex, AudioTrackType));
        } else {
            MAIN.undoStack()->push(
                new Timeline::InsertTrackCommand(m_model, trackIndex, VideoTrackType));
        }
    }
}

void TimelineDock::chooseClipAtPosition(int position, int &trackIndex, int &clipIndex)
{
    QScopedPointer<Mlt::Producer> clip;

    // Start by checking for a hit at the specified track
    if (trackIndex != -1 && !isTrackLocked(trackIndex)) {
        clipIndex = clipIndexAtPosition(trackIndex, position);
        if (clipIndex != -1 && !isBlank(trackIndex, clipIndex))
            return;
    }

    // Next we try the current track
    trackIndex = currentTrack();
    clipIndex = qMin(clipIndexAtPosition(trackIndex, position), clipCount(trackIndex) - 1);

    if (!isTrackLocked(trackIndex) && clipIndex != -1 && !isBlank(trackIndex, clipIndex)) {
        return;
    }

    // if there was no hit, look through the other tracks
    for (trackIndex = 0; trackIndex < m_model.trackList().size(); (trackIndex)++) {
        if (trackIndex == currentTrack())
            continue;
        if (isTrackLocked(trackIndex))
            continue;
        clipIndex = clipIndexAtPosition(trackIndex, position);
        if (clipIndex != -1 && !isBlank(trackIndex, clipIndex))
            return;
    }

    // As last resort choose blank on current track
    trackIndex = currentTrack();
    if (!isTrackLocked(trackIndex)) {
        clipIndex = clipIndexAtPosition(trackIndex, position);
        if (clipIndex != -1)
            return;
    }

    trackIndex = -1;
    clipIndex = -1;
}

int TimelineDock::clipCount(int trackIndex) const
{
    if (trackIndex < 0)
        trackIndex = currentTrack();
    if (trackIndex >= 0 && trackIndex < m_model.trackList().size()) {
        int i = m_model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            return playlist.count();
        }
    }
    return 0;
}

void TimelineDock::setCurrentTrack(int currentTrack)
{
    if (currentTrack != m_currentTrack) {
        m_currentTrack = currentTrack;
        emit currentTrackChanged();
    }
}

int TimelineDock::currentTrack() const
{
    return m_currentTrack;
}

void TimelineDock::setSelectionFromJS(const QVariantList &list)
{
    QList<QPoint> points;
    for (const auto &v : list) {
        auto p = v.toPoint();
        if (!isBlank(p.y(), p.x()))
            points << p;
    }
    setSelection(points);
}

void TimelineDock::setSelection(QList<QPoint> newSelection, int trackIndex, bool isMultitrack)
{
    if (!m_blockSetSelection)
        if (newSelection != selection()
                || trackIndex != m_selection.selectedTrack
                || isMultitrack != m_selection.isMultitrackSelected) {
            LOG_DEBUG() << "Changing selection to" << newSelection << " trackIndex" << trackIndex <<
                        "isMultitrack" << isMultitrack;
            m_selection.selectedClips = newSelection;
            m_selection.selectedTrack = trackIndex;
            m_selection.isMultitrackSelected = isMultitrack;
            emit selectionChanged();

            if (!m_selection.selectedClips.isEmpty())
                emitSelectedFromSelection();
            else
                emit selected(nullptr);
        }
}

QVariantList TimelineDock::selectionForJS() const
{
    QVariantList result;
    foreach (auto point, selection())
        result << QVariant(point);
    return result;
}

const QList<QPoint> TimelineDock::selection() const
{
    if (!m_quickView.rootObject())
        return QList<QPoint>();
    return m_selection.selectedClips;
}

const QVector<QUuid> TimelineDock::selectionUuids()
{
    QVector<QUuid> result;
    for (const auto &clip : selection()) {
        QScopedPointer<Mlt::ClipInfo> info(getClipInfo(clip.y(), clip.x()));
        if (info && info->cut && info->cut->is_valid())
            result << MLT.ensureHasUuid(*info->cut);
    }
    return result;
}

void TimelineDock::saveAndClearSelection()
{
    m_savedSelectedTrack = m_selection.selectedTrack;
    m_savedIsMultitrackSelected = m_selection.isMultitrackSelected;
    m_savedSelectionUuids = selectionUuids();
    m_selection.selectedClips = QList<QPoint>();
    m_selection.selectedTrack = -1;
    m_selection.isMultitrackSelected = false;
    emit selectionChanged();
}

void TimelineDock::restoreSelection()
{
    m_selection.selectedClips = QList<QPoint>();
    m_selection.selectedTrack = m_savedSelectedTrack;
    m_selection.isMultitrackSelected = m_savedIsMultitrackSelected;
    for (const auto &uuid : m_savedSelectionUuids) {
        int trackIndex, clipIndex;
        QScopedPointer<Mlt::ClipInfo> info(m_model.findClipByUuid(uuid, trackIndex, clipIndex));
        if (info) {
            m_selection.selectedClips << QPoint(clipIndex, trackIndex);
        }
    }
    emit selectionChanged();
    emitSelectedFromSelection();
}

void TimelineDock::selectClipUnderPlayhead()
{
    int track = -1, clip = -1;
    chooseClipAtPosition(m_position, track, clip);
    if (clip == -1) {
        if (isTrackLocked(currentTrack())) {
            emit warnTrackLocked(currentTrack());
            return;
        }
        int idx = clipIndexAtPlayhead(-1);
        if (idx == -1)
            setSelection();
        else
            setSelection(QList<QPoint>() << QPoint(idx, track));
        return;
    }

    if (track != -1) {
        setCurrentTrack(track);
        setSelection(QList<QPoint>() << QPoint(clip, track));
    }
}

int TimelineDock::centerOfClip(int trackIndex, int clipIndex)
{
    QScopedPointer<Mlt::ClipInfo> clip(getClipInfo(trackIndex, clipIndex));
    return clip ? clip->start + clip->frame_count / 2 : -1;
}

bool TimelineDock::isTrackLocked(int trackIndex) const
{
    if (trackIndex < 0 || trackIndex >= m_model.trackList().size())
        return false;
    int i = m_model.trackList().at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
    return track->get_int(kTrackLockProperty);
}

void TimelineDock::trimClipAtPlayhead(TrimLocation location, bool ripple)
{
    int trackIndex = currentTrack(), clipIndex = -1;
    chooseClipAtPosition(m_position, trackIndex, clipIndex);
    if (trackIndex < 0 || clipIndex < 0)
        return;
    setCurrentTrack(trackIndex);

    int i = m_model.trackList().at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
    if (!track)
        return;

    QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
    if (!info)
        return;

    if (location == TrimInPoint) {
        MAIN.undoStack()->push(
            new Timeline::TrimClipInCommand(m_model, m_markersModel, trackIndex, clipIndex,
                                            m_position - info->start, ripple));
        if (ripple)
            setPosition(info->start);
        if (m_updateCommand && m_updateCommand->trackIndex() == trackIndex
                && m_updateCommand->clipIndex() == clipIndex)
            m_updateCommand->setPosition(trackIndex, clipIndex,
                                         m_updateCommand->position() + m_position - info->start);
    } else {
        MAIN.undoStack()->push(
            new Timeline::TrimClipOutCommand(m_model, m_markersModel, trackIndex, clipIndex,
                                             info->start + info->frame_count - m_position, ripple));
        if (m_updateCommand && m_updateCommand->trackIndex() == trackIndex
                && m_updateCommand->clipIndex() == clipIndex)
            m_updateCommand->setPosition(trackIndex, clipIndex, -1);
    }
}

void TimelineDock::openProperties()
{
    MAIN.onPropertiesDockTriggered(true);
}

void TimelineDock::emitSelectedChanged(const QVector<int> &roles)
{
    if (selection().isEmpty())
        return;
    auto point = selection().first();
    auto index = model()->makeIndex(point.y(), point.x());
    emit model()->dataChanged(index, index, roles);
}

void TimelineDock::clearSelectionIfInvalid()
{
    QList<QPoint> newSelection;
    foreach (auto clip, selection()) {
        if (clip.x() >= clipCount(clip.y()))
            continue;

        newSelection << QPoint(clip.x(), clip.y());
    }
    setSelection(newSelection);
}

void TimelineDock::insertTrack()
{
    if (m_selection.selectedTrack != -1)
        setSelection();
    MAIN.undoStack()->push(
        new Timeline::InsertTrackCommand(m_model, currentTrack()));
}

void TimelineDock::insertAudioTrack()
{
    if (m_selection.selectedTrack != -1)
        setSelection();
    MAIN.undoStack()->push(
        new Timeline::InsertTrackCommand(m_model, currentTrack(), AudioTrackType));
}

void TimelineDock::insertVideoTrack()
{
    if (m_selection.selectedTrack != -1)
        setSelection();
    MAIN.undoStack()->push(
        new Timeline::InsertTrackCommand(m_model, currentTrack(), VideoTrackType));
}

void TimelineDock::removeTrack()
{
    if (m_model.trackList().size() > 0) {
        int trackIndex = currentTrack();
        MAIN.undoStack()->push(
            new Timeline::RemoveTrackCommand(m_model, trackIndex));
        if (trackIndex >= m_model.trackList().count())
            setCurrentTrack(m_model.trackList().count() - 1);
    }
}

void TimelineDock::moveTrack(int fromTrackIndex, int toTrackIndex)
{
    const TrackList &trackList = m_model.trackList();
    if (fromTrackIndex >= trackList.size()) {
        LOG_DEBUG() << "From track index out of bounds" << fromTrackIndex;
        return;
    }
    if (toTrackIndex >= trackList.size()) {
        LOG_DEBUG() << "To track index out of bounds" << toTrackIndex;
        return;
    }
    if (trackList[fromTrackIndex].type != trackList[toTrackIndex].type) {
        LOG_DEBUG() << "From/To track types do not match";
        return;
    }
    MAIN.undoStack()->push(new Timeline::MoveTrackCommand(m_model, fromTrackIndex, toTrackIndex));
    setCurrentTrack(toTrackIndex);
}

void TimelineDock::moveTrackUp()
{
    int trackIndex = currentTrack();
    const TrackList &trackList = m_model.trackList();
    if (trackIndex >= trackList.size()) {
        LOG_DEBUG() << "Track Index out of bounds" << trackIndex;
        return;
    }
    if (trackList[trackIndex].type == VideoTrackType) {
        bool topVideo = true;
        foreach (const Track &t, trackList) {
            if (t.type == VideoTrackType && t.number > trackList[trackIndex].number) {
                topVideo = false;
                break;
            }
        }
        if (topVideo) {
            MAIN.showStatusMessage(tr("Track %1 was not moved").arg(m_model.getTrackName(trackIndex)));
            return;
        }
    }
    if (trackList[trackIndex].number == 0 && trackList[trackIndex].type == AudioTrackType) {
        MAIN.showStatusMessage(tr("Can not move audio track above video track"));
        return;
    }
    MAIN.undoStack()->push(new Timeline::MoveTrackCommand(m_model, trackIndex, trackIndex - 1));
    setCurrentTrack(trackIndex - 1);
}

void TimelineDock::moveTrackDown()
{
    int trackIndex = currentTrack();
    const TrackList &trackList = m_model.trackList();
    if (trackIndex >= trackList.size()) {
        LOG_DEBUG() << "Track Index out of bounds" << trackIndex;
        return;
    }
    if (trackList[trackIndex].number == 0 && trackList[trackIndex].type == VideoTrackType) {
        MAIN.showStatusMessage(tr("Can not move video track below audio track"));
        return;
    }
    if (trackList[trackIndex].type == AudioTrackType) {
        bool bottomAudio = true;
        foreach (const Track &t, trackList) {
            if (t.type == AudioTrackType && t.number > trackList[trackIndex].number) {
                bottomAudio = false;
                break;
            }
        }
        if (bottomAudio) {
            MAIN.showStatusMessage(tr("Track %1 was not moved").arg(m_model.getTrackName(trackIndex)));
            return;
        }
    }
    MAIN.undoStack()->push(new Timeline::MoveTrackCommand(m_model, trackIndex, trackIndex + 1));
    setCurrentTrack(trackIndex + 1);
}

bool TimelineDock::mergeClipWithNext(int trackIndex, int clipIndex, bool dryrun)
{
    if (dryrun)
        return m_model.mergeClipWithNext(trackIndex, clipIndex, true);

    MAIN.undoStack()->push(
        new Timeline::MergeCommand(m_model, trackIndex, clipIndex));

    return true;
}

void TimelineDock::onProducerChanged(Mlt::Producer *after)
{
    int trackIndex = currentTrack();
    if (trackIndex < 0 || selection().isEmpty() || !m_updateCommand || !after || !after->is_valid())
        return;
    if (isTrackLocked(trackIndex)) {
        emit warnTrackLocked(trackIndex);
        return;
    }
    int i = m_model.trackList().at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
    if (track) {
        // Ensure the new XML has same in/out point as selected clip by making
        // a copy of the changed producer and copying the in/out from timeline.
        Mlt::Playlist playlist(*track);
        int clipIndex = selection().first().x();
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
        if (info) {
            double oldSpeed = qstrcmp("timewarp",
                                      info->producer->get("mlt_service")) ? 1.0 : info->producer->get_double("warp_speed");
            double newSpeed = qstrcmp("timewarp",
                                      after->get("mlt_service")) ? 1.0 : after->get_double("warp_speed");
            double speedRatio = oldSpeed / newSpeed;

            int length = qRound(info->length * speedRatio);
            int in = qMin(qRound(info->frame_in * speedRatio), length - 1);
            int out = qMin(qRound(info->frame_out * speedRatio), length - 1);
            if (!Settings.timelineRipple() && (clipIndex + 1) < playlist.count()) {
                // limit the out point to what fits before the next clip
                if (playlist.is_blank(clipIndex + 1)) {
                    out = qMin(out, in + info->frame_count - 1 + playlist.clip_length(clipIndex + 1));
                } else {
                    out = qMin(out, in + info->frame_count - 1);
                }
            }
            after->set_in_and_out(in, out);

            // Adjust filters.
            int n = after->filter_count();
            for (int j = 0; j < n; j++) {
                QScopedPointer<Mlt::Filter> filter(after->filter(j));
                if (filter && filter->is_valid() && !filter->get_int("_loader")) {
                    in = qMin(qRound(filter->get_in() * speedRatio), length - 1);
                    out = qMin(qRound(filter->get_out() * speedRatio), length - 1);
                    filter->set_in_and_out(in, out);
                    //TODO: keyframes
                }
            }

            if (speedRatio != 1.0 && Settings.timelineRipple() && Settings.timelineRippleAllTracks()
                    && (clipIndex + 1) < playlist.count()) {
                auto position = info->start + out - in + 1;
                QScopedPointer<Mlt::ClipInfo> nextInfo(playlist.clip_info(clipIndex + 1));
                if (playlist.is_blank(clipIndex + 1)) {
                    position += nextInfo->frame_count;
                    nextInfo.reset(playlist.clip_info(clipIndex + 2));
                }
                if (nextInfo && nextInfo->cut) {
                    MAIN.undoStack()->beginMacro(tr("Change clip properties"));
                    MAIN.undoStack()->push(
                        new Timeline::LiftCommand(m_model, trackIndex, clipIndex));
                    auto moveCommand = new Timeline::MoveClipCommand(m_model, m_markersModel, 0, true);
                    nextInfo->cut->set(kPlaylistStartProperty, position);
                    moveCommand->selection().insert(nextInfo->start, *nextInfo->cut);
                    MAIN.undoStack()->push(moveCommand);
                    MAIN.undoStack()->push(
                        new Timeline::OverwriteCommand(m_model, trackIndex, info->start, MLT.XML(after), false));
                    MAIN.undoStack()->endMacro();
                    return;
                }
            }
        }
    }
    QString xmlAfter = MLT.XML(after);
    m_updateCommand->setXmlAfter(xmlAfter);
    setSelection(); // clearing selection prevents a crash
    MAIN.undoStack()->push(m_updateCommand.take());
}

void TimelineDock::addAudioTrack()
{
    if (m_selection.selectedTrack != -1)
        setSelection();
    MAIN.undoStack()->push(
        new Timeline::AddTrackCommand(m_model, false));
}

void TimelineDock::addVideoTrack()
{
    if (m_selection.selectedTrack != -1)
        setSelection();
    MAIN.undoStack()->push(
        new Timeline::AddTrackCommand(m_model, true));
}

void TimelineDock::alignSelectedClips()
{
    auto selection = selectionUuids();
    saveAndClearSelection();
    AlignAudioDialog dialog(tr("Align To Reference Track"), &m_model, selection, this);
    dialog.exec();
    restoreSelection();
}

void TimelineDock::onShowFrame(const SharedFrame &frame)
{
    if (m_ignoreNextPositionChange) {
        m_ignoreNextPositionChange = false;
    } else if (MLT.isMultitrack() && m_position != frame.get_position()) {
        m_position = qMin(frame.get_position(), m_model.tractor()->get_length());
        emit positionChanged();
    }
}

void TimelineDock::onSeeked(int position)
{
    if (MLT.isMultitrack() && m_position != position) {
        m_position = qMin(position, m_model.tractor()->get_length());
        emit positionChanged();
    }
}


static bool isSystemClipboardValid(const QString &xml)
{
    return MLT.isMltXml(xml) && MAIN.isClipboardNewer() && !xml.contains(kShotcutFiltersClipboard);
}

void TimelineDock::append(int trackIndex)
{
    if (trackIndex < 0)
        trackIndex = currentTrack();
    if (isTrackLocked(trackIndex)) {
        emit warnTrackLocked(trackIndex);
        return;
    }
    if (MAIN.isSourceClipMyProject()) return;

    // Use MLT XML on the clipboard if it exists and is newer than source clip.
    QString xmlToUse = QGuiApplication::clipboard()->text();
    if (isSystemClipboardValid(xmlToUse)) {
        if (!Settings.proxyEnabled()) {
            ProxyManager::filterXML(xmlToUse, "");
        }
    } else {
        xmlToUse.clear();
    }

    if (MLT.isSeekableClip() || MLT.savedProducer() || !xmlToUse.isEmpty()) {
        if (xmlToUse.isEmpty()) {
            Mlt::Producer producer(MLT.isClip() ? MLT.producer() : MLT.savedProducer());
            ProxyManager::generateIfNotExists(producer);
            xmlToUse = MLT.XML(&producer);
        }
        if (xmlToUse.isEmpty()) {
            return;
        }

        // Insert multiple if the XML is a <tractor> with child <property name="shotcut">1</property>
        // No need to create a track in an empty timeline.
        // This can be a macro of QUndoCommands.
        Mlt::Producer producer(MLT.profile(), "xml-string", xmlToUse.toUtf8().constData());
        if (producer.is_valid() && producer.type() == mlt_service_tractor_type
                && producer.get_int(kShotcutXmlProperty)) {
            Mlt::Tractor tractor(producer);
            Mlt::ClipInfo info;
            MAIN.undoStack()->beginMacro(tr("Append multiple to timeline"));
            Mlt::Controller::RefreshBlocker blocker;

            // Loop over each source track
            for (int mltTrackIndex = 0; mltTrackIndex < tractor.count(); mltTrackIndex++) {
                QScopedPointer<Mlt::Producer> srcTrack(tractor.track(mltTrackIndex));
                if (srcTrack) {
                    const auto trackIndex = currentTrack() + mltTrackIndex;
                    addTrackIfNeeded(trackIndex, srcTrack.get());

                    // Insert the clips for this track
                    Mlt::Playlist playlist(*srcTrack);
                    for (int mltClipIndex = 0; mltClipIndex < playlist.count(); mltClipIndex++) {
                        if (!playlist.is_blank(mltClipIndex)) {
                            playlist.clip_info(mltClipIndex, &info);
                            Mlt::Producer clip(info.producer);
                            clip.set_in_and_out(info.frame_in, info.frame_out);
                            MAIN.undoStack()->push(
                                new Timeline::AppendCommand(m_model, trackIndex, MLT.XML(&clip)));
                        }
                    }
                }
            }
            MAIN.undoStack()->endMacro();
            MLT.refreshConsumer();

        } else {
            if (m_model.trackList().size() == 0) {
                addVideoTrack();
            }

            MAIN.undoStack()->push(
                new Timeline::AppendCommand(m_model, trackIndex, xmlToUse));
        }

        if (m_position < 0) {
            // This happens when pasting in a new session
            MAIN.openCut(new Mlt::Producer(m_model.tractor()));
        }
    } else if (!MLT.isSeekableClip()) {
        emitNonSeekableWarning();
    }
}

void TimelineDock::remove(int trackIndex, int clipIndex)
{
    if (!m_model.trackList().count())
        return;
    if (isTrackLocked(trackIndex)) {
        emit warnTrackLocked(trackIndex);
        return;
    }
    Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
    Mlt::Producer clip = producerForClip(trackIndex, clipIndex);
    if (clip.is_valid()) {
        MAIN.undoStack()->push(
            new Timeline::RemoveCommand(m_model, m_markersModel, trackIndex, clipIndex));
    }
}

void TimelineDock::lift(int trackIndex, int clipIndex)
{
    if (!m_model.trackList().count())
        return;
    if (isTrackLocked(trackIndex)) {
        emit warnTrackLocked(trackIndex);
        return;
    }
    if (trackIndex < 0 || clipIndex < 0) return;
    Mlt::Producer clip(producerForClip(trackIndex, clipIndex));
    if (clip.is_valid()) {
        if (clip.is_blank())
            return;
        MAIN.undoStack()->push(
            new Timeline::LiftCommand(m_model, trackIndex, clipIndex));
        setSelection();
    }
}

void TimelineDock::removeSelection(bool withCopy)
{
    if (isTrackLocked(currentTrack())) {
        emit warnTrackLocked(currentTrack());
        return;
    }
    if (selection().isEmpty())
        selectClipUnderPlayhead();
    if (selection().isEmpty() || currentTrack() < 0)
        return;

    // Cut
    if (withCopy) {
        auto clip = selection().first();
        copy(clip.y(), clip.x());
        if (selection().size() < 2) {
            remove(clip.y(), clip.x());
            return;
        }
    }

    // Ripple delete
    int n = selection().size();
    if (n > 1) {
        if (withCopy)
            MAIN.undoStack()->beginMacro(tr("Cut %1 from timeline").arg(n));
        else
            MAIN.undoStack()->beginMacro(tr("Remove %1 from timeline").arg(n));
    }
    int trackIndex, clipIndex;
    for (const auto &uuid : selectionUuids()) {
        delete m_model.findClipByUuid(uuid, trackIndex, clipIndex);
        remove(trackIndex, clipIndex);
    }
    if (n > 1)
        MAIN.undoStack()->endMacro();
}

void TimelineDock::liftSelection()
{
    if (isTrackLocked(currentTrack())) {
        emit warnTrackLocked(currentTrack());
        return;
    }
    if (selection().isEmpty())
        selectClipUnderPlayhead();
    if (selection().isEmpty())
        return;
    int n = selection().size();
    if (n > 1)
        MAIN.undoStack()->beginMacro(tr("Lift %1 from timeline").arg(n));
    int trackIndex, clipIndex;
    for (const auto &uuid : selectionUuids()) {
        delete m_model.findClipByUuid(uuid, trackIndex, clipIndex);
        lift(trackIndex, clipIndex);
    }
    if (n > 1)
        MAIN.undoStack()->endMacro();
}

void TimelineDock::incrementCurrentTrack(int by)
{
    int newTrack = currentTrack();
    if (by < 0)
        newTrack = qMax(0, newTrack + by);
    else
        newTrack = qMin(m_model.trackList().size() - 1, newTrack + by);
    setCurrentTrack(newTrack);
}

void TimelineDock::selectTrackHead(int trackIndex)
{
    if (trackIndex >= 0) {
        setSelection(QList<QPoint>(), trackIndex);
        int i = m_model.trackList().at(trackIndex).mlt_index;
        Mlt::Producer *producer = m_model.tractor()->track(i);
        if (producer && producer->is_valid()) {
            producer->set(kTrackIndexProperty, trackIndex);
            emit selected(producer);
        }
        delete producer;
    }
}

void TimelineDock::selectMultitrack()
{
    setSelection(QList<QPoint>(), -1, true);
    emit multitrackSelected();
    emit selected(m_model.tractor());
}


template<typename T>
static void insertSorted(std::vector<T> &vec, T const &item)
{
    vec.insert(std::upper_bound(vec.begin(), vec.end(), item), item);
}

void TimelineDock::copy(int trackIndex, int clipIndex)
{
    auto selected = selection();
    if (selected.size() < 2) {
        if (trackIndex < 0)
            trackIndex = currentTrack();
        if (clipIndex < 0)
            clipIndex = clipIndexAtPlayhead(trackIndex);
        Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
        QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
        if (info) {
            QString xml = MLT.XML(info->producer);
            Mlt::Producer p(MLT.profile(), "xml-string", xml.toUtf8().constData());
            p.set_speed(0);
            p.seek(info->frame_in);
            p.set_in_and_out(info->frame_in, info->frame_out);
            MLT.setSavedProducer(&p);
            QGuiApplication::clipboard()->setText(MLT.XML(&p));
            emit clipCopied();
        }
    } else {
        // Determine the track indices
        auto minY = std::numeric_limits<int>::max();
        auto maxY = -1;
        auto minStart = std::numeric_limits<int>::max();
        for (auto &a : selected) {
            minY = std::min(minY, a.y());
            maxY = std::max(maxY, a.y());
            auto info = getClipInfo(a.y(), a.x());
            if (info) minStart = std::min(minStart, info->start);
            delete info;
        }
        // Create the tracks
        Mlt::Tractor tractor(MLT.profile());
        tractor.set(kShotcutXmlProperty, 1);
        for (int trackIndex = minY, i = 0; trackIndex <= maxY; trackIndex++, i++) {
            Mlt::Playlist playlist(MLT.profile());
            if (m_model.trackList()[trackIndex].type == AudioTrackType) {
                playlist.set("hide", 1);
                playlist.set(kAudioTrackProperty, 1);
            } else {
                playlist.set(kVideoTrackProperty, 1);
            }
            tractor.set_track(playlist, i);

            // Sort all the clips on this track
            std::vector<int> clipIndices;
            for (auto &a : selected) {
                if (a.y() == trackIndex) {
                    clipIndices.insert(std::upper_bound(clipIndices.begin(), clipIndices.end(), a.x()), a.x());
                }
            }

            // Add the clips to the tracks
            if (clipIndices.size() > 0) {
                int prevEnd = minStart;
                auto mlt_index = m_model.trackList()[trackIndex].mlt_index;
                QScopedPointer<Mlt::Producer> sourceTrack(m_model.tractor()->track(mlt_index));
                if (sourceTrack) {
                    Mlt::Playlist sourcePlaylist(*sourceTrack);
                    Mlt::ClipInfo info;
                    for (auto clipIndex : clipIndices) {
                        sourcePlaylist.clip_info(clipIndex, &info);
                        playlist.blank(info.start - prevEnd - 1);
                        playlist.append(*info.producer, info.frame_in, info.frame_out);
                        prevEnd = info.start + info.frame_count;
                    }
                }
            }
        }
        // Put XML in clipboard
        QGuiApplication::clipboard()->setText(MLT.XML(&tractor));
    }
}

void TimelineDock::emitSelectedFromSelection()
{
    if (!m_model.trackList().count()) {
        if (m_model.tractor())
            selectMultitrack();
        else
            emit selected(nullptr);
        return;
    }

    int trackIndex = selection().isEmpty() ? currentTrack() : selection().first().y();
    int clipIndex  = selection().isEmpty() ? 0              : selection().first().x();
    QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
    if (info && info->producer && info->producer->is_valid()) {
        m_updateCommand.reset(new Timeline::UpdateCommand(*this, trackIndex, clipIndex, info->start));
        // We need to set these special properties so time-based filters
        // can get information about the cut while still applying filters
        // to the cut parent.
        QScopedPointer<Mlt::ClipInfo> info2(getClipInfo(trackIndex, clipIndex - 1));
        if (info2 && info2->producer && info2->producer->is_valid()
                && info2->producer->get(kShotcutTransitionProperty)) {
            // Factor in a transition left of the clip.
            info->producer->set(kFilterInProperty, info->frame_in - info2->frame_count);
            info->producer->set(kPlaylistStartProperty, info2->start);
        } else {
            info->producer->set(kFilterInProperty, info->frame_in);
            info->producer->set(kPlaylistStartProperty, info->start);
        }
        info2.reset(getClipInfo(trackIndex, clipIndex + 1));
        if (info2 && info2->producer && info2->producer->is_valid()
                && info2->producer->get(kShotcutTransitionProperty)) {
            // Factor in a transition right of the clip.
            info->producer->set(kFilterOutProperty, info->frame_out + info2->frame_count);
        } else {
            info->producer->set(kFilterOutProperty, info->frame_out);
        }
        info->producer->set(kMultitrackItemProperty,
                            QString("%1:%2").arg(clipIndex).arg(trackIndex).toLatin1().constData());
        m_ignoreNextPositionChange = true;
        emit selected(info->producer);
    }
    m_model.tractor()->set(kFilterInProperty, 0);
    m_model.tractor()->set(kFilterOutProperty, m_model.tractor()->get_length() - 1);
}

void TimelineDock::remakeAudioLevels(int trackIndex, int clipIndex, bool force)
{
    if (Settings.timelineShowWaveforms()) {
        QModelIndex modelIndex = m_model.index(clipIndex, 0, m_model.index(trackIndex));
        QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
        AudioLevelsTask::start(*info->producer, &m_model, modelIndex, force);
    }
}

void TimelineDock::commitTrimCommand()
{
    if (m_trimCommand && (m_trimDelta || m_transitionDelta)) {
        if (m_undoHelper) m_trimCommand->setUndoHelper(m_undoHelper.take());
        MAIN.undoStack()->push(m_trimCommand.take());
    }
    m_trimDelta = 0;
    m_transitionDelta = 0;
}

void TimelineDock::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent)
    // Adjust selected clips for changed indices.
    if (-1 == m_selection.selectedTrack) {
        QList<QPoint> newSelection;
        int n = last - first + 1;
        if (parent.isValid()) {
            foreach (auto i, m_selection.selectedClips) {
                if (i.x() < first)
                    newSelection << QPoint(i.x(), parent.row());
                else
                    newSelection << QPoint(i.x() + n, parent.row());
            }
        } else {
            foreach (auto i, m_selection.selectedClips) {
                if (i.y() < first)
                    newSelection << QPoint(i.x(), i.y());
                else
                    newSelection << QPoint(i.x(), i.y() + n);
            }
        }
        setSelection(newSelection);
        if (!parent.isValid())
            model()->reload(true);
    }
}

void TimelineDock::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent)
    // Adjust selected clips for changed indices.
    if (-1 == m_selection.selectedTrack && parent.isValid()) {
        QList<QPoint> newSelection;
        int n = last - first + 1;
        if (parent.isValid()) {
            foreach (auto i, m_selection.selectedClips) {
                if (i.x() < first)
                    newSelection << QPoint(i.x(), parent.row());
                else if (i.x() > last)
                    newSelection << QPoint(i.x() - n, parent.row());
            }
        } else {
            foreach (auto i, m_selection.selectedClips) {
                if (i.y() < first)
                    newSelection << QPoint(i.x(), i.y());
                else if (i.y() > last)
                    newSelection << QPoint(i.x(), i.y() - n);
            }
        }
        setSelection(newSelection);
        if (!parent.isValid())
            model()->reload(true);
    }
}

void TimelineDock::onRowsMoved(const QModelIndex &parent, int start, int end,
                               const QModelIndex &destination, int row)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    Q_UNUSED(destination)
    Q_UNUSED(row)
    // Workaround issue in timeline qml that clip selection becomes inconsistent with the model
    // Clear the selection and reload the model to trigger reset of the selected clips in the UI
    QList<QPoint> newSelection;
    setSelection(newSelection);
    model()->reload(true);
}

void TimelineDock::detachAudio(int trackIndex, int clipIndex)
{
    if (!m_model.trackList().count())
        return;
    Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
    QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
    if (info && info->producer && info->producer->is_valid() && !info->producer->is_blank()
            && info->producer->get("audio_index") && info->producer->get_int("audio_index") >= 0) {
        if (!info->producer->property_exists(kDefaultAudioIndexProperty)) {
            info->producer->set(kDefaultAudioIndexProperty, info->producer->get_int("audio_index"));
        }
        Mlt::Producer clip(MLT.profile(), "xml-string", MLT.XML(info->producer).toUtf8().constData());
        clip.set_in_and_out(info->frame_in, info->frame_out);
        MAIN.undoStack()->push(
            new Timeline::DetachAudioCommand(m_model, trackIndex, clipIndex, info->start, MLT.XML(&clip)));
    }
}

void TimelineDock::selectAll()
{
    QList<QPoint> selection;
    for (int y = 0; y < m_model.rowCount(); y++) {
        for (int x = 0; x < m_model.rowCount(m_model.index(y)); x++) {
            if (!isBlank(y, x) && !isTrackLocked(y))
                selection << QPoint(x, y);
        }
    }
    setSelection(selection);
}

void TimelineDock::selectAllOnCurrentTrack()
{
    int y = currentTrack();
    QList<QPoint> selection;
    if (y > -1 && y < m_model.rowCount()) {
        for (int x = 0; x < m_model.rowCount(m_model.index(y)); x++) {
            if (!isBlank(y, x) && !isTrackLocked(y))
                selection << QPoint(x, y);
        }
    }
    setSelection(selection);
}

bool TimelineDock::blockSelection(bool block)
{
    m_blockSetSelection = block;
    return m_blockSetSelection;
}

void TimelineDock::onProducerModified()
{
    // The clip name may have changed.
    emitSelectedChanged(QVector<int>() << MultitrackModel::NameRole << MultitrackModel::CommentRole);
}

void TimelineDock::replace(int trackIndex, int clipIndex, const QString &xml)
{
    if (xml.isEmpty() && !MLT.isClip() && !MLT.savedProducer()) {
        emit showStatusMessage(tr("There is nothing in the Source player."));
        return;
    }
    if (!m_model.trackList().count() || MAIN.isSourceClipMyProject())
        return;
    if (trackIndex < 0)
        trackIndex = currentTrack();
    if (isTrackLocked(trackIndex)) {
        emit warnTrackLocked(trackIndex);
        return;
    }
    if (clipIndex < 0)
        clipIndex = clipIndexAtPlayhead(trackIndex);
    Mlt::Producer producer(producerForClip(trackIndex, clipIndex));
    if (producer.is_valid() && producer.type() == mlt_service_tractor_type) {
        emit showStatusMessage(tr("You cannot replace a transition."));
        return;
    }
    if (MLT.isSeekableClip() || MLT.savedProducer() || !xml.isEmpty()) {
        Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
        QString xmlToUse = !xml.isEmpty() ? xml
                           : MLT.XML(MLT.isClip() ? nullptr : MLT.savedProducer());
        MAIN.undoStack()->push(
            new Timeline::ReplaceCommand(m_model, trackIndex, clipIndex, xmlToUse));
    } else if (!MLT.isSeekableClip()) {
        emitNonSeekableWarning();
    }
}

void TimelineDock::createOrEditMarker()
{
    if (!m_model.trackList().count() || MLT.producer()->get_length() <= 1)
        return;
    int index = m_markersModel.markerIndexForPosition(m_position);
    if (index >= 0) {
        editMarker(index);
        return;
    }
    createMarker();
}

void TimelineDock::createOrEditSelectionMarker()
{
    auto selected = selection();
    if (!m_model.trackList().count() || MLT.producer()->get_length() <= 1 || selected.isEmpty()) {
        emit showStatusMessage(tr("Select a clip in the timeline to create a marker around it"));
        return;
    }

    // Find the earliest start and the latest end in the selection
    int start = std::numeric_limits<int>::max();
    int end = std::numeric_limits<int>::min();
    for (const auto &clip : selected) {
        QScopedPointer<Mlt::ClipInfo> info(getClipInfo(clip.y(), clip.x()));
        if (info) {
            if (info->start < start) {
                start = info->start;
            }
            if ((info->start + info->frame_count) > end) {
                end = info->start + info->frame_count;
            }
        }
    }

    if (start != std::numeric_limits<int>::max()) {
        int index = m_markersModel.markerIndexForRange(start, end);
        if (index >= 0) {
            editMarker(index);
            return;
        } else {
            Markers::Marker marker;
            marker.text = QString("Marker %1").arg(m_markersModel.uniqueKey() + 1);
            marker.color = Settings.markerColor();
            marker.start = start;
            marker.end = end;
            m_markersModel.append(marker);
            emit showStatusMessage(tr("Added marker: \"%1\".").arg(marker.text));
            return;
        }
    }
}

void TimelineDock::createMarker()
{
    if (!m_model.trackList().count() || MLT.producer()->get_length() <= 1)
        return;
    int index = m_markersModel.markerIndexForPosition(m_position);
    if (index >= 0) {
        return;
    }
    Markers::Marker marker;
    marker.text = QString("Marker %1").arg(m_markersModel.uniqueKey() + 1);
    marker.color = Settings.markerColor();
    marker.start = position();
    marker.end = position();
    m_markersModel.append(marker);
    emit showStatusMessage(tr("Added marker: \"%1\". Hold %2 and drag to create a range")
                           .arg(marker.text, QmlApplication::OS() == "OS X" ? "⌘" : "Ctrl"));
}

void TimelineDock::editMarker(int markerIndex)
{
    Markers::Marker marker = m_markersModel.getMarker(markerIndex);
    EditMarkerDialog dialog(this, marker.text, marker.color, marker.start, marker.end,
                            m_model.tractor()->get_length() - 1);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (dialog.exec() == QDialog::Accepted) {
        marker.text = dialog.getText();
        marker.color = dialog.getColor();
        marker.start = dialog.getStart();
        marker.end = dialog.getEnd();
        m_markersModel.update(markerIndex, marker);
    }
}

void TimelineDock::deleteMarker(int markerIndex)
{
    if (markerIndex < 0) {
        markerIndex = m_markersModel.markerIndexForPosition(m_position);
    }
    if (markerIndex >= 0) {
        m_markersModel.remove(markerIndex);
    }
}

void TimelineDock::seekNextMarker()
{
    int nextPos = m_markersModel.nextMarkerPosition(m_position);
    if (nextPos >= 0) {
        setPosition(nextPos);
        markerSeeked(m_markersModel.markerIndexForPosition(nextPos));
    }
}

void TimelineDock::seekPrevMarker()
{
    int prevPos = m_markersModel.prevMarkerPosition(m_position);
    if (prevPos >= 0) {
        setPosition(prevPos);
        markerSeeked(m_markersModel.markerIndexForPosition(prevPos));
    }
}

void TimelineDock::onFilterModelChanged()
{
    if (m_updateCommand) {
        m_updateCommand->setPosition(-1, -1, -1);
    }
}

void TimelineDock::setTrackName(int trackIndex, const QString &value)
{
    MAIN.undoStack()->push(
        new Timeline::NameTrackCommand(m_model, trackIndex, value));
}

void TimelineDock::toggleTrackMute(int trackIndex)
{
    MAIN.undoStack()->push(
        new Timeline::MuteTrackCommand(m_model, trackIndex));
}

void TimelineDock::toggleTrackHidden(int trackIndex)
{
    MAIN.undoStack()->push(
        new Timeline::HideTrackCommand(m_model, trackIndex));
}

void TimelineDock::setTrackComposite(int trackIndex, bool composite)
{
    MAIN.undoStack()->push(
        new Timeline::CompositeTrackCommand(m_model, trackIndex, composite));
}

void TimelineDock::setTrackLock(int trackIndex, bool lock)
{
    MAIN.undoStack()->push(
        new Timeline::LockTrackCommand(m_model, trackIndex, lock));
}

bool TimelineDock::moveClip(int fromTrack, int toTrack, int clipIndex, int position, bool ripple)
{
    if (toTrack >= 0 && clipIndex >= 0) {
        int length = 0;
        int i = m_model.trackList().at(fromTrack).mlt_index;
        Mlt::Producer track(m_model.tractor()->track(i));
        if (track.is_valid()) {
            Mlt::Playlist playlist(track);
            length = playlist.clip_length(clipIndex);
        }
        i = m_model.trackList().at(toTrack).mlt_index;
        track = Mlt::Producer(m_model.tractor()->track(i));
        if (track.is_valid()) {
            Mlt::Playlist playlist(track);
            if (m_model.isTransition(playlist, playlist.get_clip_index_at(position)) ||
                    m_model.isTransition(playlist, playlist.get_clip_index_at(position + length - 1))) {
                return false;
            }
        }
    }
    if (selection().size() <= 1
            && m_model.addTransitionValid(fromTrack, toTrack, clipIndex, position, ripple)) {
        emit transitionAdded(fromTrack, clipIndex, position, ripple);
        if (m_updateCommand)
            m_updateCommand->setPosition(toTrack, clipIndex, position);
    } else {
        // Check for locked tracks
        auto trackDelta = toTrack - fromTrack;
        for (const auto &clip : selection()) {
            auto trackIndex = clip.y() + trackDelta;
            if (isTrackLocked(clip.y())) {
                emit warnTrackLocked(clip.y());
                return false;
            }
            if (isTrackLocked(trackIndex)) {
                emit warnTrackLocked(trackIndex);
                return false;
            }
        }

        // Workaround bug #326 moving clips between tracks stops allowing drag-n-drop
        // into Timeline, which appeared with Qt 5.6 upgrade.
        emit clipMoved(fromTrack, toTrack, clipIndex, position, ripple);
        if (m_updateCommand)
            m_updateCommand->setPosition(toTrack, clipIndex, position);
    }
    return true;
}

void TimelineDock::onClipMoved(int fromTrack, int toTrack, int clipIndex, int position, bool ripple)
{
    int n = selection().size();
    if (n > 0) {
        // determine the position delta
        for (const auto &clip : selection()) {
            if (clip.y() == fromTrack && clip.x() == clipIndex) {
                QScopedPointer<Mlt::ClipInfo> info(getClipInfo(clip.y(), clip.x()));
                if (info) {
                    position -= info->start;
                    break;
                }
            }
        }
        auto command = new Timeline::MoveClipCommand(m_model, m_markersModel, toTrack - fromTrack, ripple);

        // Copy selected
        for (const auto &clip : selection()) {
            QScopedPointer<Mlt::ClipInfo> info(getClipInfo(clip.y(), clip.x()));
            if (info && info->cut) {
                LOG_DEBUG() << "moving clip at" << clip << "start" << info->start << "+" << position << "=" <<
                            info->start + position;
                info->cut->set(kPlaylistStartProperty, info->start + position);
                command->selection().insert(info->start, *info->cut);
            }
        }
        setSelection();
        TimelineSelectionBlocker blocker(*this);
        MAIN.undoStack()->push(command);
    }
}

bool TimelineDock::trimClipIn(int trackIndex, int clipIndex, int oldClipIndex, int delta,
                              bool ripple)
{
    if (!ripple && m_model.addTransitionByTrimInValid(trackIndex, clipIndex, delta)) {
        clipIndex = m_model.addTransitionByTrimIn(trackIndex, clipIndex, delta);
        m_transitionDelta += delta;
        m_trimCommand.reset(new Timeline::AddTransitionByTrimInCommand(m_model, trackIndex, clipIndex - 1,
                                                                       m_transitionDelta, m_trimDelta, false));
        if (m_updateCommand && m_updateCommand->trackIndex() == trackIndex
                && m_updateCommand->clipIndex() == clipIndex)
            m_updateCommand->setPosition(trackIndex, clipIndex, -1);
    } else if (!ripple && m_model.removeTransitionByTrimInValid(trackIndex, clipIndex, delta)) {
        Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
        QModelIndex modelIndex = m_model.makeIndex(trackIndex, clipIndex - 1);
        int n = m_model.data(modelIndex, MultitrackModel::DurationRole).toInt();
        m_model.liftClip(trackIndex, clipIndex - 1);
        if (delta < 0 ) {
            m_model.trimClipIn(trackIndex, clipIndex, -n, false, false);
            m_trimDelta += -n;
        } else if (delta > 0) {
            m_model.trimClipOut(trackIndex, clipIndex - 2, -n, false, false);
            m_transitionDelta = 0;
        }
        m_trimCommand.reset(new Timeline::RemoveTransitionByTrimInCommand(m_model, trackIndex,
                                                                          clipIndex - 1, m_trimDelta, false));
        if (m_updateCommand && m_updateCommand->trackIndex() == trackIndex
                && m_updateCommand->clipIndex() == clipIndex)
            m_updateCommand->setPosition(trackIndex, clipIndex - 1, -1);
    } else if (!ripple && m_model.trimTransitionOutValid(trackIndex, clipIndex, delta)) {
        m_model.trimTransitionOut(trackIndex, clipIndex, delta);
        m_trimDelta += delta;
        m_trimCommand.reset(new Timeline::TrimTransitionOutCommand(m_model, trackIndex, clipIndex,
                                                                   m_trimDelta, false));
    } else if (m_model.trimClipInValid(trackIndex, clipIndex, delta, ripple)) {
        if (!m_undoHelper) {
            m_undoHelper.reset(new UndoHelper(m_model));
            if (!ripple) {
                m_undoHelper->setHints(UndoHelper::SkipXML);
            } else {
                m_undoHelper->setHints(UndoHelper::RestoreTracks);
            }
            m_undoHelper->recordBeforeState();
        }
        clipIndex = m_model.trimClipIn(trackIndex, clipIndex, delta, ripple,
                                       Settings.timelineRippleAllTracks());

        m_trimDelta += delta;
        m_trimCommand.reset(new Timeline::TrimClipInCommand(m_model, m_markersModel, trackIndex,
                                                            oldClipIndex, m_trimDelta, ripple, false));
        if (m_updateCommand && m_updateCommand->trackIndex() == trackIndex
                && m_updateCommand->clipIndex() == clipIndex)
            m_updateCommand->setPosition(trackIndex, clipIndex, m_updateCommand->position() + delta);
    } else return false;

    // Update duration in properties
    QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
    if (info && !info->producer->get_int(kShotcutSequenceProperty))
        emit durationChanged();

    return true;
}

bool TimelineDock::trimClipOut(int trackIndex, int clipIndex, int delta, bool ripple)
{
    if (!ripple && m_model.addTransitionByTrimOutValid(trackIndex, clipIndex, delta)) {
        m_model.addTransitionByTrimOut(trackIndex, clipIndex, delta);
        m_transitionDelta += delta;
        m_trimCommand.reset(new Timeline::AddTransitionByTrimOutCommand(m_model, trackIndex, clipIndex,
                                                                        m_transitionDelta, m_trimDelta, false));
        if (m_updateCommand && m_updateCommand->trackIndex() == trackIndex
                && m_updateCommand->clipIndex() == clipIndex)
            m_updateCommand->setPosition(trackIndex, clipIndex, -1);
    } else if (!ripple && m_model.removeTransitionByTrimOutValid(trackIndex, clipIndex, delta)) {
        Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
        QModelIndex modelIndex = m_model.makeIndex(trackIndex, clipIndex + 1);
        int n = m_model.data(modelIndex, MultitrackModel::DurationRole).toInt();
        m_model.liftClip(trackIndex, clipIndex + 1);
        if (delta < 0 ) {
            m_model.trimClipOut(trackIndex, clipIndex, -n, false, false);
            m_trimDelta += -n;
        } else if (delta > 0) {
            m_model.trimClipIn(trackIndex, clipIndex + 2, -n, false, false);
            m_transitionDelta = 0;
        }
        m_trimCommand.reset(new Timeline::RemoveTransitionByTrimOutCommand(m_model, trackIndex,
                                                                           clipIndex + 1, m_trimDelta, false));
        if (m_updateCommand && m_updateCommand->trackIndex() == trackIndex
                && m_updateCommand->clipIndex() == clipIndex)
            m_updateCommand->setPosition(trackIndex, clipIndex, -1);
    } else if (!ripple && m_model.trimTransitionInValid(trackIndex, clipIndex, delta)) {
        m_model.trimTransitionIn(trackIndex, clipIndex, delta);
        m_trimDelta += delta;
        m_trimCommand.reset(new Timeline::TrimTransitionInCommand(m_model, trackIndex, clipIndex,
                                                                  m_trimDelta, false));
    } else if (m_model.trimClipOutValid(trackIndex, clipIndex, delta, ripple)) {
        if (!m_undoHelper) {
            m_undoHelper.reset(new UndoHelper(m_model));
            if (!ripple) m_undoHelper->setHints(UndoHelper::SkipXML);
            m_undoHelper->recordBeforeState();
        }
        m_model.trimClipOut(trackIndex, clipIndex, delta, ripple, Settings.timelineRippleAllTracks());

        m_trimDelta += delta;
        m_trimCommand.reset(new Timeline::TrimClipOutCommand(m_model, m_markersModel, trackIndex, clipIndex,
                                                             m_trimDelta, ripple, false));
        if (m_updateCommand && m_updateCommand->trackIndex() == trackIndex
                && m_updateCommand->clipIndex() == clipIndex)
            m_updateCommand->setPosition(trackIndex, clipIndex, -1);
    } else return false;

    // Update duration in properties
    QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
    if (info && !info->producer->get_int(kShotcutSequenceProperty))
        emit durationChanged();

    return true;
}

static QString convertUrlsToXML(const QString &xml)
{
    if (xml.startsWith(kFileUrlProtocol)) {
        LongUiTask longTask(QObject::tr("Drop Files"));
        Mlt::Playlist playlist(MLT.profile());
        QList<QUrl> urls;
        const auto &strings = xml.split(kFilesUrlDelimiter);
        for (auto s : strings) {
#ifdef Q_OS_WIN
            if (!s.startsWith(kFileUrlProtocol)) {
                s.prepend(kFileUrlProtocol);
            }
#endif
            QUrl url(s);
            urls << Util::removeFileScheme(url);
        }
        int i = 0, count = urls.size();
        for (const auto &path : Util::sortedFileList(urls)) {
            if (MAIN.isSourceClipMyProject(path, /* withDialog */ false)) continue;
            if (MLT.checkFile(path)) {
                MAIN.showStatusMessage(QObject::tr("Failed to open ").append(path));
                continue;
            }
            longTask.reportProgress(Util::baseName(path), i++, count);
            Mlt::Producer p;
            if (path.endsWith(".mlt") || path.endsWith(".xml")) {
                p = Mlt::Producer(MLT.profile(), "xml", path.toUtf8().constData());
                if (p.is_valid()) {
                    p.set(kShotcutVirtualClip, 1);
                    p.set("resource", path.toUtf8().constData());
                }
            } else {
                p = Mlt::Producer(MLT.profile(), path.toUtf8().constData());
            }
            if (p.is_valid()) {
                if (!qstrcmp(p.get("mlt_service"), "avformat") && !p.get_int("seekable")) {
                    MAIN.showStatusMessage(QObject::tr("Not adding non-seekable file: ") + Util::baseName(path));
                    continue;
                }
                Mlt::Producer *producer = MLT.setupNewProducer(&p);
                ProxyManager::generateIfNotExists(*producer);
                playlist.append(*producer);
                delete producer;
            }
        }
        return MLT.XML(&playlist);
    }
    return xml;
}

void TimelineDock::insert(int trackIndex, int position, const QString &xml, bool seek)
{
    // Validations
    if (trackIndex < 0)
        trackIndex = currentTrack();
    if (isTrackLocked(trackIndex)) {
        emit warnTrackLocked(trackIndex);
        return;
    }
    if (xml.contains(MAIN.fileName()) && MAIN.isSourceClipMyProject()) return;

    // Handle drop from file manager to empty project.
    if ((!MLT.producer() || !MLT.producer()->is_valid()) && xml.startsWith(kFileUrlProtocol)) {
        QUrl url = xml.split(kFilesUrlDelimiter).first();
        Mlt::Properties properties;
        properties.set(kShotcutSkipConvertProperty, 1);
        if (!MAIN.open(Util::removeFileScheme(url), &properties, false /* play */))
            MAIN.open(Util::removeFileScheme(url, false), &properties, false /* play */);
    }

    // Use MLT XML on the clipboard if it exists and is newer than source clip.
    QString xmlToUse = QGuiApplication::clipboard()->text();
    if (isSystemClipboardValid(xmlToUse)) {
        if (!Settings.proxyEnabled()) {
            ProxyManager::filterXML(xmlToUse, "");
        }
    } else {
        xmlToUse.clear();
    }

    if (MLT.isSeekableClip() || MLT.savedProducer() || !xml.isEmpty() || !xmlToUse.isEmpty()) {
        QScopedPointer<TimelineSelectionBlocker> selectBlocker;
        if (xmlToUse.isEmpty() && xml.isEmpty()) {
            Mlt::Producer producer(MLT.isClip() ? MLT.producer() : MLT.savedProducer());
            ProxyManager::generateIfNotExists(producer);
            xmlToUse = MLT.XML(&producer);
        } else if (!xml.isEmpty()) {
            // Convert a list of file URLs from the xml arg to MLT XML
            xmlToUse = convertUrlsToXML(xml);
            if (xml.startsWith(kFileUrlProtocol) && xml.split(kFilesUrlDelimiter).size() > 1) {
                selectBlocker.reset(new TimelineSelectionBlocker(*this));
            }
        }
        if (xmlToUse.isEmpty()) {
            return;
        }
        if (position < 0) {
            position = qMax(m_position, 0);
        }

        // Insert multiple if the XML is a <tractor> with child <property name="shotcut">1</property>
        // No need to create a track in an empty timeline.
        // This can be a macro of QUndoCommands.
        Mlt::Producer producer(MLT.profile(), "xml-string", xmlToUse.toUtf8().constData());
        if (producer.is_valid() && producer.type() == mlt_service_tractor_type
                && producer.get_int(kShotcutXmlProperty)) {
            Mlt::Tractor tractor(producer);
            Mlt::ClipInfo info;
            MAIN.undoStack()->beginMacro(tr("Insert multiple into timeline"));
            Mlt::Controller::RefreshBlocker blocker;

            // Loop over each source track
            for (int mltTrackIndex = 0; mltTrackIndex < tractor.count(); mltTrackIndex++) {
                QScopedPointer<Mlt::Producer> srcTrack(tractor.track(mltTrackIndex));
                if (srcTrack) {
                    const auto trackIndex = currentTrack() + mltTrackIndex;
                    addTrackIfNeeded(trackIndex, srcTrack.get());

                    // Insert the clips for this track
                    Mlt::Playlist playlist(*srcTrack);
                    for (int mltClipIndex = 0; mltClipIndex < playlist.count(); mltClipIndex++) {
                        if (!playlist.is_blank(mltClipIndex)) {
                            playlist.clip_info(mltClipIndex, &info);
                            Mlt::Producer clip(info.producer);
                            clip.set_in_and_out(info.frame_in, info.frame_out);
                            MAIN.undoStack()->push(
                                new Timeline::InsertCommand(m_model, m_markersModel, trackIndex, position + info.start,
                                                            MLT.XML(&clip), seek));
                        }
                    }
                }
            }
            MAIN.undoStack()->endMacro();

        } else {
            if (m_model.trackList().size() == 0) {
                position = 0;
                addVideoTrack();
            }
            MAIN.undoStack()->push(
                new Timeline::InsertCommand(m_model, m_markersModel, trackIndex, position, xmlToUse, seek));
        }
        if (m_position < 0) {
            // This happens when pasting in a new session
            MAIN.openCut(new Mlt::Producer(m_model.tractor()));
        }
    } else if (!MLT.isSeekableClip()) {
        emitNonSeekableWarning();
    }
}

void TimelineDock::selectClip(int trackIndex, int clipIndex)
{
    setSelection(QList<QPoint>() << QPoint(clipIndex, trackIndex));
}

void TimelineDock::onMultitrackClosed()
{
    stopRecording();
    m_position = -1;
    m_ignoreNextPositionChange = false;
    m_trimDelta = 0;
    m_transitionDelta = 0;
    m_blockSetSelection = false;
    setSelection();
    setZoom(1.0);
}

void TimelineDock::reloadTimelineMarkers()
{
    m_markersModel.load(m_model.tractor());
}

void TimelineDock::overwrite(int trackIndex, int position, const QString &xml, bool seek)
{
    // Validations
    if (trackIndex < 0)
        trackIndex = currentTrack();
    if (isTrackLocked(trackIndex)) {
        emit warnTrackLocked(trackIndex);
        return;
    }
    if (xml.contains(MAIN.fileName()) && MAIN.isSourceClipMyProject()) return;

    // Handle drop from file manager to empty project.
    if ((!MLT.producer() || !MLT.producer()->is_valid()) && xml.startsWith(kFileUrlProtocol)) {
        QUrl url = xml.split(kFilesUrlDelimiter).first();
        Mlt::Properties properties;
        properties.set(kShotcutSkipConvertProperty, 1);
        if (!MAIN.open(Util::removeFileScheme(url), &properties, false /* play */))
            MAIN.open(Util::removeFileScheme(url, false), &properties, false /* play */);
    }

    // Use MLT XML on the clipboard if it exists and is newer than source clip.
    QString xmlToUse = QGuiApplication::clipboard()->text();
    if (isSystemClipboardValid(xmlToUse)) {
        if (!Settings.proxyEnabled()) {
            ProxyManager::filterXML(xmlToUse, "");
        }
    } else {
        xmlToUse.clear();
    }

    if (MLT.isSeekableClip() || MLT.savedProducer() || !xml.isEmpty() || !xmlToUse.isEmpty()) {
        QScopedPointer<TimelineSelectionBlocker> selectBlocker;
        if (xmlToUse.isEmpty() && xml.isEmpty()) {
            Mlt::Producer producer(MLT.isClip() ? MLT.producer() : MLT.savedProducer());
            ProxyManager::generateIfNotExists(producer);
            xmlToUse = MLT.XML(&producer);
        } else if (!xml.isEmpty()) {
            xmlToUse = convertUrlsToXML(xml);
            if (xml.startsWith(kFileUrlProtocol) && xml.split(kFilesUrlDelimiter).size() > 1) {
                selectBlocker.reset(new TimelineSelectionBlocker(*this));
            }
        }
        if (position < 0) {
            position = qMax(m_position, 0);
        }

        // Overwrite multiple if the XML is a <tractor> with child <property name="shotcut">1</property>
        // No need to create a track in an empty timeline.
        // This can be a macro of QUndoCommands.
        Mlt::Producer producer(MLT.profile(), "xml-string", xmlToUse.toUtf8().constData());
        if (producer.is_valid() && producer.type() == mlt_service_tractor_type
                && producer.get_int(kShotcutXmlProperty)) {
            Mlt::Tractor tractor(producer);
            Mlt::ClipInfo info;
            MAIN.undoStack()->beginMacro(tr("Overwrite multiple onto timeline"));
            Mlt::Controller::RefreshBlocker blocker;

            // Loop over each source track
            for (int mltTrackIndex = 0; mltTrackIndex < tractor.count(); mltTrackIndex++) {
                QScopedPointer<Mlt::Producer> srcTrack(tractor.track(mltTrackIndex));
                if (srcTrack) {
                    const auto trackIndex = currentTrack() + mltTrackIndex;
                    addTrackIfNeeded(trackIndex, srcTrack.get());

                    // Insert the clips for this track
                    Mlt::Playlist playlist(*srcTrack);
                    for (int mltClipIndex = 0; mltClipIndex < playlist.count(); mltClipIndex++) {
                        if (!playlist.is_blank(mltClipIndex)) {
                            playlist.clip_info(mltClipIndex, &info);
                            Mlt::Producer clip(info.producer);
                            clip.set_in_and_out(info.frame_in, info.frame_out);
                            MAIN.undoStack()->push(
                                new Timeline::OverwriteCommand(m_model, trackIndex, position + info.start, MLT.XML(&clip), seek));
                        }
                    }
                }
            }
            MAIN.undoStack()->endMacro();

        } else {
            if (m_model.trackList().size() == 0) {
                position = 0;
                addVideoTrack();
            }

            MAIN.undoStack()->push(
                new Timeline::OverwriteCommand(m_model, trackIndex, position, xmlToUse, seek));
        }
        if (m_position < 0) {
            // This happens when pasting in a new session
            MAIN.openCut(new Mlt::Producer(m_model.tractor()));
        }
    } else if (!MLT.isSeekableClip()) {
        emitNonSeekableWarning();
    }
}

void TimelineDock::appendFromPlaylist(Mlt::Playlist *playlist, bool skipProxy)
{
    int trackIndex = currentTrack();
    if (trackIndex < 0) {
        trackIndex = 0;
    }
    if (isTrackLocked(trackIndex)) {
        emit warnTrackLocked(trackIndex);
        return;
    }
    // Workaround a bug with first slide of slideshow animation not working.
    if (skipProxy) {
        // Initialize the multitrack with a bogus clip and remove it.
        Mlt::Producer producer(playlist->get_clip(0));
        auto clipIndex = m_model.appendClip(trackIndex, producer);
        if (clipIndex >= 0)
            m_model.removeClip(trackIndex, clipIndex, Settings.timelineRippleAllTracks());
    }
    disconnect(&m_model, &MultitrackModel::appended, this, &TimelineDock::selectClip);
    MAIN.undoStack()->push(
        new Timeline::AppendCommand(m_model, trackIndex, MLT.XML(playlist), skipProxy));
    connect(&m_model, &MultitrackModel::appended, this, &TimelineDock::selectClip,
            Qt::QueuedConnection);
}

void TimelineDock::splitClip(int trackIndex, int clipIndex)
{
    if (trackIndex < 0 || clipIndex < 0)
        chooseClipAtPosition(m_position, trackIndex, clipIndex);
    if (trackIndex < 0 || clipIndex < 0)
        return;
    setCurrentTrack(trackIndex);
    if (clipIndex >= 0 && trackIndex >= 0) {
        int i = m_model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            if (!m_model.isTransition(playlist, clipIndex)) {
                QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
                if (info && m_position > info->start && m_position < info->start + info->frame_count) {
                    setSelection(); // Avoid filter views becoming out of sync
                    MAIN.undoStack()->push(
                        new Timeline::SplitCommand(m_model, trackIndex, clipIndex, m_position));
                }
            } else {
                emit showStatusMessage(tr("You cannot split a transition."));
            }
        }
    }
}

void TimelineDock::fadeIn(int trackIndex, int clipIndex, int duration)
{
    if (isTrackLocked(trackIndex)) {
        emit warnTrackLocked(trackIndex);
        return;
    }
    if (duration < 0) return;
    Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
    MAIN.undoStack()->push(
        new Timeline::FadeInCommand(m_model, trackIndex, clipIndex, duration));
    emit fadeInChanged(duration);
}

void TimelineDock::fadeOut(int trackIndex, int clipIndex, int duration)
{
    if (isTrackLocked(trackIndex)) {
        emit warnTrackLocked(trackIndex);
        return;
    }
    if (duration < 0) return;
    Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
    MAIN.undoStack()->push(
        new Timeline::FadeOutCommand(m_model, trackIndex, clipIndex, duration));
    emit fadeOutChanged(duration);
}

void TimelineDock::seekPreviousEdit()
{
    if (!MLT.isMultitrack()) return;
    if (!m_model.tractor()) return;

    int newPosition = -1;
    int n = m_model.tractor()->count();
    for (int i = 0; i < n; i++) {
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            int clipIndex = playlist.get_clip_index_at(m_position);
            if (clipIndex >= 0 && m_position == playlist.clip_start(clipIndex))
                --clipIndex;
            if (clipIndex >= 0)
                newPosition = qMax(newPosition, playlist.clip_start(clipIndex));
        }
    }
    if (newPosition != m_position)
        setPosition(newPosition);
}

void TimelineDock::seekNextEdit()
{
    if (!MLT.isMultitrack()) return;
    if (!m_model.tractor()) return;

    int newPosition = std::numeric_limits<int>::max();
    int n = m_model.tractor()->count();
    for (int i = 0; i < n; i++) {
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            int clipIndex = playlist.get_clip_index_at(m_position) + 1;
            if (clipIndex < playlist.count())
                newPosition = qMin(newPosition, playlist.clip_start(clipIndex));
            else if (clipIndex == playlist.count())
                newPosition = qMin(newPosition, playlist.clip_start(clipIndex) + playlist.clip_length(clipIndex));
        }
    }
    if (newPosition != m_position)
        setPosition(newPosition);
}

void TimelineDock::seekInPoint(int clipIndex)
{
    if (!MLT.isMultitrack()) return;
    if (!m_model.tractor()) return;
    if (clipIndex < 0) return;

    int mltTrackIndex = m_model.trackList().at(currentTrack()).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(mltTrackIndex));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (m_position != playlist.clip_start(clipIndex))
            setPosition(playlist.clip_start(clipIndex));
    }
}

void TimelineDock::dragEnterEvent(QDragEnterEvent *event)
{
    LOG_DEBUG() << event->mimeData()->hasFormat(Mlt::XmlMimeType);
    if (event->mimeData()->hasFormat(Mlt::XmlMimeType)) {
        event->acceptProposedAction();
    }
}

void TimelineDock::dragMoveEvent(QDragMoveEvent *event)
{
    emit dragging(event->posF(), event->mimeData()->text().toInt());
}

void TimelineDock::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    emit dropped();
}

void TimelineDock::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat(Mlt::XmlMimeType)) {
        int trackIndex = currentTrack();
        if (trackIndex >= 0) {
            emit dropAccepted(QString::fromUtf8(event->mimeData()->data(Mlt::XmlMimeType)));
            event->acceptProposedAction();
        }
    }
    emit dropped();
}

bool TimelineDock::event(QEvent *event)
{
    bool result = QDockWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        load(true);
    return result;
}

void TimelineDock::keyPressEvent(QKeyEvent *event)
{
    QDockWidget::keyPressEvent(event);
    if (!event->isAccepted())
        MAIN.keyPressEvent(event);
}

void TimelineDock::keyReleaseEvent(QKeyEvent *event)
{
    QDockWidget::keyReleaseEvent(event);
    if (!event->isAccepted())
        MAIN.keyReleaseEvent(event);
}

void TimelineDock::load(bool force)
{
    if (m_quickView.source().isEmpty() || force) {
        int saveCurrentTrack = -1;
        if (!m_quickView.source().isEmpty())
            saveCurrentTrack = currentTrack();
        QDir sourcePath = QmlUtilities::qmlDir();
        sourcePath.cd("views");
        sourcePath.cd("timeline");
        m_quickView.setFocusPolicy(isFloating() ? Qt::NoFocus : Qt::StrongFocus);
        m_quickView.setSource(QUrl::fromLocalFile(sourcePath.filePath("timeline.qml")));
        connect(m_quickView.rootObject(), SIGNAL(clipClicked()), this, SIGNAL(clipClicked()));
        connect(m_quickView.rootObject(), SIGNAL(timelineRightClicked()),  this,
                SLOT(onTimelineRightClicked()));
        connect(m_quickView.rootObject(), SIGNAL(clipRightClicked()), this, SLOT(onClipRightClicked()));
        if (force && Settings.timelineShowWaveforms())
            m_model.reload();
        if (saveCurrentTrack != -1)
            setCurrentTrack(saveCurrentTrack);
    } else if (Settings.timelineShowWaveforms()) {
        m_model.reload();
    }
}

void TimelineDock::onTopLevelChanged(bool floating)
{
    m_quickView.setFocusPolicy(floating ? Qt::NoFocus : Qt::StrongFocus);
}

void TimelineDock::onTransitionAdded(int trackIndex, int clipIndex, int position, bool ripple)
{
    setSelection(); // cleared
    Timeline::AddTransitionCommand *command = new Timeline::AddTransitionCommand(*this, trackIndex,
                                                                                 clipIndex, position, ripple);
    MAIN.undoStack()->push(command);
    // Select the transition.
    setSelection(QList<QPoint>() << QPoint(command->getTransitionIndex(), trackIndex));
}

void TimelineDock::onTimelineRightClicked()
{
    m_mainMenu->popup(QCursor::pos());
}

void TimelineDock::onClipRightClicked()
{
    m_clipMenu->popup(QCursor::pos());
}

class FindProducersByHashParser : public Mlt::Parser
{
private:
    QString m_hash;
    QList<Mlt::Producer> m_producers;

public:
    FindProducersByHashParser(const QString &hash)
        : Mlt::Parser()
        , m_hash(hash)
    {}

    QList<Mlt::Producer> &producers()
    {
        return m_producers;
    }

    int on_start_filter(Mlt::Filter *)
    {
        return 0;
    }
    int on_start_producer(Mlt::Producer *producer)
    {
        if (producer->is_cut() && Util::getHash(producer->parent()) == m_hash)
            m_producers << Mlt::Producer(producer);
        return 0;
    }
    int on_end_producer(Mlt::Producer *)
    {
        return 0;
    }
    int on_start_playlist(Mlt::Playlist *)
    {
        return 0;
    }
    int on_end_playlist(Mlt::Playlist *)
    {
        return 0;
    }
    int on_start_tractor(Mlt::Tractor *)
    {
        return 0;
    }
    int on_end_tractor(Mlt::Tractor *)
    {
        return 0;
    }
    int on_start_multitrack(Mlt::Multitrack *)
    {
        return 0;
    }
    int on_end_multitrack(Mlt::Multitrack *)
    {
        return 0;
    }
    int on_start_track()
    {
        return 0;
    }
    int on_end_track()
    {
        return 0;
    }
    int on_end_filter(Mlt::Filter *)
    {
        return 0;
    }
    int on_start_transition(Mlt::Transition *)
    {
        return 0;
    }
    int on_end_transition(Mlt::Transition *)
    {
        return 0;
    }
    int on_start_chain(Mlt::Chain *)
    {
        return 0;
    }
    int on_end_chain(Mlt::Chain *)
    {
        return 0;
    }
    int on_start_link(Mlt::Link *)
    {
        return 0;
    }
    int on_end_link(Mlt::Link *)
    {
        return 0;
    }
};

void TimelineDock::replaceClipsWithHash(const QString &hash, Mlt::Producer &producer)
{
    FindProducersByHashParser parser(hash);
    parser.start(*model()->tractor());
    auto n = parser.producers().size();
    if (n > 1)
        MAIN.undoStack()->beginMacro(tr("Replace %n timeline clips", nullptr, n));
    for (auto &clip : parser.producers()) {
        int trackIndex = -1;
        int clipIndex = -1;
        // lookup the current track and clip index by UUID
        QScopedPointer<Mlt::ClipInfo> info(MAIN.timelineClipInfoByUuid(clip.get(kUuidProperty), trackIndex,
                                                                       clipIndex));

        if (info && info->producer->is_valid() && trackIndex >= 0 && clipIndex >= 0
                && info->producer->type() != mlt_service_tractor_type) {
            if (producer.get_int(kIsProxyProperty) && info->producer->get_int(kIsProxyProperty)) {
                // Not much to do on a proxy clip but change its resource
                info->producer->set(kOriginalResourceProperty, producer.get("resource"));
                auto caption = Util::baseName(ProxyManager::resource(*info->producer), true);
                if (!::qstrcmp(info->producer->get("mlt_service"), "timewarp")) {
                    caption = QString("%1 (%2x)").arg(caption, info->producer->get("warp_speed"));
                }
                info->producer->set(kShotcutCaptionProperty, caption.toUtf8().constData());
            } else {
                int in = clip.get_in();
                int out = clip.get_out();

                // Factor in a transition left of the clip.
                QScopedPointer<Mlt::ClipInfo> info2(getClipInfo(trackIndex, clipIndex - 1));
                if (info2 && info2->producer && info2->producer->is_valid()
                        && info2->producer->get(kShotcutTransitionProperty)) {
                    in -= info2->frame_count;
                }
                // Factor in a transition right of the clip.
                info2.reset(getClipInfo(trackIndex, clipIndex + 1));
                if (info2 && info2->producer && info2->producer->is_valid()
                        && info2->producer->get(kShotcutTransitionProperty)) {
                    out += info2->frame_count;
                }
                Util::applyCustomProperties(producer, *info->producer, in, out);

                replace(trackIndex, clipIndex, MLT.XML(&producer));
            }
        }
    }
    if (n > 1)
        MAIN.undoStack()->endMacro();
}

void TimelineDock::recordAudio()
{
    // Get the file name.
    auto filename = QmlApplication::getNextProjectFile("voiceover.opus");
    if (filename.isEmpty()) {
        QString path = Settings.savePath();
        path.append("/%1.opus");
        path = path.arg(tr("voiceover"));
        auto nameFilter = tr("Opus (*.opus);;All Files (*)");
        filename = QFileDialog::getSaveFileName(this, tr("Record Audio"), path, nameFilter,
                                                nullptr, Util::getFileDialogOptions());
    }
    if (filename.isEmpty()) {
        return;
    }
    if (!filename.endsWith(".opus")) {
        filename += ".opus";
    }
    auto info = QFileInfo(filename);
    Settings.setSavePath(info.path());
    MAIN.undoStack()->beginMacro(tr("Record Audio: %1").arg(info.fileName()));

    // See if current track is audio track with no clips at playhead and beyond.
    auto trackIndex = currentTrack();
    bool addTrack = false;
    if (trackIndex >= 0 && trackIndex < m_model.trackList().size() &&
            m_model.trackList().at(trackIndex).type == AudioTrackType) {
        auto clipIndex = clipIndexAtPosition(trackIndex, position());
        addTrack = clipIndex != -1 &&
                   (!isBlank(trackIndex, clipIndex) || clipIndex < clipCount(trackIndex) - 1);
    } else {
        addTrack = true;
    }
    // Add audio track if needed.
    if (addTrack) {
        addAudioTrack();
        trackIndex = m_model.trackList().size() - 1;
        setCurrentTrack(trackIndex);
    }

    // Add renamed color clip to audio track.
    auto clip = Mlt::Producer(MLT.profile(), "color:");
    clip.set(kShotcutCaptionProperty, info.fileName().toUtf8().constData());
    clip.set(kShotcutDetailProperty, filename.toUtf8().constData());
    clip.set(kBackgroundCaptureProperty, 1);
    clip.set("length", std::numeric_limits<int>::max());
    clip.set_in_and_out(0, 0);
    overwrite(trackIndex, -1, MLT.XML(&clip), false);
    m_recordingTrackIndex = trackIndex;
    m_recordingClipIndex = clipIndexAtPosition(trackIndex, position());

    // Start ffmpeg background job.
    auto priority = QThread::HighPriority;
#if defined(Q_OS_MAC)
    QStringList args {"-f", "avfoundation", "-i", "none:" + Settings.audioInput()};
    priority = QThread::NormalPriority;
#elif defined(Q_OS_WIN)
    QStringList args {"-f", "dshow", "-i", "audio=" + Settings.audioInput()};
#else
    QStringList args {"-f", "pulse", "-name", "Shotuct", "-i", Settings.audioInput()};
#endif
    args << "-flush_packets" << "1" << "-y" << filename;
    m_recordJob.reset(new FfmpegJob("vo", args, false, priority));
    connect(m_recordJob.data(), SIGNAL(started()), SLOT(onRecordStarted()));
    connect(m_recordJob.data(), SIGNAL(finished(AbstractJob *, bool)),
            SLOT(onRecordFinished(AbstractJob *, bool)));
    m_recordJob->start();
    m_isRecording = true;
    emit isRecordingChanged(m_isRecording);
}

void TimelineDock::onRecordStarted()
{
    // Use a timer to increase length of color clip.
    m_recordingTimer.setInterval(kRecordingTimerIntervalMs);
    connect(&m_recordingTimer, SIGNAL(timeout()), this, SLOT(updateRecording()));
    m_recordingTime = QDateTime::currentDateTime();
    m_recordingTimer.start();

    // Start playback.
    MLT.play();
}

void TimelineDock::updateRecording()
{
    int out = qRound(MLT.profile().fps() * m_recordingTime.secsTo(QDateTime::currentDateTime()));
    std::unique_ptr<Mlt::ClipInfo> info(getClipInfo(m_recordingTrackIndex, m_recordingClipIndex));
    if (info) {
        auto delta = info->frame_out - out;
        if (delta < 0) {
            m_model.trimClipOut(m_recordingTrackIndex, m_recordingClipIndex, delta, false, false);
        }
    }
}

void TimelineDock::onRecordFinished(AbstractJob *, bool success)
{
    if (!success) {
        stopRecording();
        Settings.setAudioInput(QString()); // saved input likely no longer valid
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        emit showStatusMessage(tr("Record Audio error: check PulseAudio settings"));
#else
        emit showStatusMessage(tr("Record Audio error: choose File > Open Other > Audio/Video Device"));
#endif
    }
}

void TimelineDock::stopRecording()
{
    m_recordingTimer.stop();

    if (m_isRecording) {
        m_isRecording = false;
        emit isRecordingChanged(m_isRecording);

        // Stop ffmpeg job.
        if (m_recordJob && m_recordJob->state() != QProcess::NotRunning) {
            m_recordJob->stop();

            // Stop playback.
            MLT.pause();

            // Wait for ffmpeg to flush the recording.
            LongUiTask longTask(tr("Record Audio"));
            longTask.setMinimumDuration(500);
            QFuture<int> future = QtConcurrent::run([]() {
                QThread::msleep(3000);
                return 0;
            });
            longTask.wait<int>(tr("Saving audio recording..."), future);
        }

        // Replace color clip.
        std::unique_ptr<Mlt::ClipInfo> info(getClipInfo(m_recordingTrackIndex, m_recordingClipIndex));
        if (info && info->producer && info->producer->is_valid()) {
            Mlt::Producer clip(MLT.profile(), info->producer->get(kShotcutDetailProperty));
            lift(m_recordingTrackIndex, m_recordingClipIndex);
            if (clip.is_valid()) {
                overwrite(m_recordingTrackIndex, info->start, MLT.XML(&clip), false);
            }
        }
        MAIN.undoStack()->endMacro();
    }
}
