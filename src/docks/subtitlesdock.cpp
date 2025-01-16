/*
 * Copyright (c) 2024 Meltytech, LLC
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

#include "subtitlesdock.h"

#include "actions.h"
#include "mainwindow.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include "dialogs/subtitletrackdialog.h"
#include "dialogs/transcribeaudiodialog.h"
#include "jobqueue.h"
#include "jobs/meltjob.h"
#include "jobs/whisperjob.h"
#include "models/subtitlesmodel.h"
#include "models/subtitlesselectionmodel.h"
#include "widgets/docktoolbar.h"
#include "qmltypes/qmlapplication.h"
#include <Logger.h>

#include "MltPlaylist.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QTextDocumentFragment>
#include <QTextEdit>
#include <QTreeView>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtWidgets/QScrollArea>

#define DEFAULT_ITEM_DURATION (2 * 1000)

static int64_t positionToMs(mlt_position position)
{
    auto seconds = position / MLT.profile().fps();
    return 1000 * seconds;
}

static mlt_position msToPosition(int64_t ms)
{
    return ms * MLT.profile().frame_rate_num() / MLT.profile().frame_rate_den() / 1000;
}

static QList<Subtitles::SubtitleItem> readSrtFile(const QString &path, int64_t timeOffset,
                                                  bool includeNonspoken)
{
    QList<Subtitles::SubtitleItem> items;

    // Read the subtitles
    Subtitles::SubtitleVector srtItems = Subtitles::readFromSrtFile(path.toUtf8().toStdString());
    if (srtItems.size() == 0) {
        return items;
    }

    // Convert the items to the return list
    Subtitles::SubtitleItem item;
    for (int i = 0; i < srtItems.size(); i++) {
        // Clean up the lines
        QStringList lines = QString::fromStdString(srtItems[i].text).split('\n');
        QString text;
        for (int i = 0; i < lines.size(); i++) {
            // Remove HTML
            QString line = QTextDocumentFragment::fromHtml(lines[i]).toPlainText();
            // Remove unnecessary space
            line = line.simplified();
            // Remove leading "-"
            if (line.startsWith("-")) {
                line = line.remove(0, 1).simplified();
            }
            // Remove unspoken sounds if requested
            if (!includeNonspoken &&
                    ((line.startsWith("[") && line.endsWith("]")) ||
                     (line.startsWith("(") && line.endsWith(")")))) {
                continue;
            }
            if (i != 0) {
                text += "\n";
            }
            text += line;
        }
        text = text.trimmed();
        if (!text.isEmpty()) {
            // Shift the subtitles to the position
            item.start = srtItems[i].start + timeOffset;
            item.end = srtItems[i].end + timeOffset;
            item.text = text.toUtf8().toStdString();
            items.append(item);
        }
    }
    return items;
}

SubtitlesDock::SubtitlesDock(QWidget *parent) :
    QDockWidget(parent)
    , m_model(nullptr)
    , m_pos(-1)
    , m_textEditInProgress(false)
{
    LOG_DEBUG() << "begin";

    setObjectName("SubtitlesDock");
    QDockWidget::setWindowTitle(tr("Subtitles"));
    QIcon filterIcon = QIcon::fromTheme("subtitle", QIcon(":/icons/oxygen/32x32/actions/subtitle.png"));
    setWindowIcon(filterIcon);
    toggleViewAction()->setIcon(windowIcon());

    setupActions();

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    QDockWidget::setWidget(scrollArea);

    QVBoxLayout *vboxLayout = new QVBoxLayout();
    scrollArea->setLayout(vboxLayout);

    m_addToTimelineLabel = new QLabel(tr("Add clips to the Timeline to begin editing subtitles."));
    vboxLayout->addWidget(m_addToTimelineLabel);

    QHBoxLayout *tracksLayout = new QHBoxLayout();
    m_trackCombo = new QComboBox();
    connect(m_trackCombo, &QComboBox::currentIndexChanged, this, [&](int trackIndex) {
        if (m_selectionModel) {
            m_selectionModel->setSelectedTrack(trackIndex);
            selectItemForTime();
        }
    });
    tracksLayout->addWidget(m_trackCombo);
    QToolButton *addTrackButton = new QToolButton(this);
    addTrackButton->setDefaultAction(Actions["subtitleAddTrackAction"]);
    addTrackButton->setAutoRaise(true);
    tracksLayout->addWidget(addTrackButton);
    QToolButton *removeTrackButton = new QToolButton(this);
    removeTrackButton->setDefaultAction(Actions["subtitleRemoveTrackAction"]);
    removeTrackButton->setAutoRaise(true);
    tracksLayout->addWidget(removeTrackButton);
    QToolButton *editTrackButton = new QToolButton(this);
    editTrackButton->setDefaultAction(Actions["subtitleEditTrackAction"]);
    editTrackButton->setAutoRaise(true);
    tracksLayout->addWidget(editTrackButton);

    vboxLayout->addLayout(tracksLayout);

    m_treeView = new QTreeView();
    vboxLayout->addWidget(m_treeView, 1);

    QMenu *mainMenu = new QMenu(tr("Subtitles"), this);
    QMenu *trackMenu = new QMenu(tr("Tracks"), this);
    trackMenu->addAction(Actions["subtitleAddTrackAction"]);
    trackMenu->addAction(Actions["subtitleRemoveTrackAction"]);
    trackMenu->addAction(Actions["subtitleEditTrackAction"]);
    mainMenu->addMenu(trackMenu);
    mainMenu->addAction(Actions["SubtitleImportAction"]);
    mainMenu->addAction(Actions["SubtitleExportAction"]);
    mainMenu->addAction(Actions["subtitleCreateEditItemAction"]);
    mainMenu->addAction(Actions["subtitleAddItemAction"]);
    mainMenu->addAction(Actions["subtitleRemoveItemAction"]);
    mainMenu->addAction(Actions["subtitleSetStartAction"]);
    mainMenu->addAction(Actions["subtitleSetEndAction"]);
    mainMenu->addAction(Actions["subtitleMoveAction"]);
    mainMenu->addAction(Actions["subtitleBurnInAction"]);
    mainMenu->addAction(Actions["subtitleGenerateTextAction"]);
    mainMenu->addAction(Actions["subtitleSpeechToTextAction"]);
    mainMenu->addAction(Actions["subtitleTrackTimelineAction"]);
    mainMenu->addAction(Actions["subtitleShowPrevNextAction"]);

    QAction *action;
    QMenu *columnsMenu = new QMenu(tr("Columns"), this);
    action = columnsMenu->addAction(tr("Start"), this, SLOT(onStartColumnToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.subtitlesShowColumn("start"));
    action = columnsMenu->addAction(tr("End"), this, SLOT(onEndColumnToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.subtitlesShowColumn("end"));
    action = columnsMenu->addAction(tr("Duration"), this, SLOT(onDurationColumnToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.subtitlesShowColumn("duration"));
    mainMenu->addMenu(columnsMenu);
    Actions.loadFromMenu(mainMenu);

    DockToolBar *toolbar = new DockToolBar(tr("Subtitle Controls"));
    toolbar->setAreaHint(Qt::BottomToolBarArea);

    auto button = new QToolButton;
    button->setIcon(QIcon::fromTheme("show-menu",
                                     QIcon(":/icons/oxygen/32x32/actions/show-menu.png")));
    button->setToolTip(tr("Subtitles Menu"));
    button->setAutoRaise(true);
    button->setMenu(mainMenu);
    button->setPopupMode(QToolButton::QToolButton::InstantPopup);
    toolbar->addWidget(button);

    button = new QToolButton;
    button->setDefaultAction(Actions["SubtitleImportAction"]);
    button->setAutoRaise(true);
    toolbar->addWidget(button);

    button = new QToolButton;
    button->setDefaultAction(Actions["subtitleAddItemAction"]);
    button->setAutoRaise(true);
    toolbar->addWidget(button);

    button = new QToolButton;
    button->setDefaultAction(Actions["subtitleRemoveItemAction"]);
    button->setAutoRaise(true);
    toolbar->addWidget(button);

    button = new QToolButton;
    button->setDefaultAction(Actions["subtitleSetStartAction"]);
    button->setAutoRaise(true);
    toolbar->addWidget(button);

    button = new QToolButton;
    button->setDefaultAction(Actions["subtitleSetEndAction"]);
    button->setAutoRaise(true);
    toolbar->addWidget(button);

    button = new QToolButton;
    button->setDefaultAction(Actions["subtitleMoveAction"]);
    button->setAutoRaise(true);
    toolbar->addWidget(button);

    button = new QToolButton;
    button->setDefaultAction(Actions["subtitleBurnInAction"]);
    button->setAutoRaise(true);
    toolbar->addWidget(button);

    button = new QToolButton;
    button->setDefaultAction(Actions["subtitleSpeechToTextAction"]);
    button->setAutoRaise(true);
    toolbar->addWidget(button);

    vboxLayout->addWidget(toolbar);

    QFontMetrics fm(font());
    int textHeight = fm.lineSpacing() * 4 + 10;
    QGridLayout *textLayout = new QGridLayout();
    m_prevLabel = new QLabel(tr("Previous"));
    textLayout->addWidget(m_prevLabel, 0, 0);
    m_textLabel = new QLabel(tr("Current"));
    textLayout->addWidget(m_textLabel, 0, 1);
    m_nextLabel = new QLabel(tr("Next"));
    textLayout->addWidget(m_nextLabel, 0, 2);
    m_prev = new QTextEdit(this);
    m_prev->setMaximumHeight(textHeight);
    m_prev->setReadOnly(true);
    m_prev->setLineWrapMode(QTextEdit::NoWrap);
    textLayout->addWidget(m_prev, 1, 0);
    m_text = new QTextEdit(this);
    m_text->setMaximumHeight(textHeight);
    m_text->setReadOnly(true);
    m_text->setTabChangesFocus(true);
    m_text->setLineWrapMode(QTextEdit::NoWrap);
    connect(m_text, &QTextEdit::textChanged, this, &SubtitlesDock::onTextEdited);
    textLayout->addWidget(m_text, 1, 1);
    m_next = new QTextEdit(this);
    m_next->setMaximumHeight(textHeight);
    m_next->setReadOnly(true);
    m_next->setLineWrapMode(QTextEdit::NoWrap);
    textLayout->addWidget(m_next, 1, 2);
    vboxLayout->addLayout(textLayout);

    vboxLayout->addStretch();

    LOG_DEBUG() << "end";
}

SubtitlesDock::~SubtitlesDock()
{
}

void SubtitlesDock::setupActions()
{
    QAction *action;

    action = new QAction(tr("Add Subtitle Track"), this);
    action->setIcon(QIcon::fromTheme("list-add",
                                     QIcon(":/icons/oxygen/32x32/actions/list-add.png")));
    action->setToolTip(tr("Add a subtitle track"));
    connect(action, &QAction::triggered, this, [&]() {
        show();
        raise();
        addSubtitleTrack();
    });
    Actions.add("subtitleAddTrackAction", action);

    action = new QAction(tr("Remove Subtitle Track"), this);
    action->setIcon(QIcon::fromTheme("list-remove",
                                     QIcon(":/icons/oxygen/32x32/actions/list-remove.png")));
    action->setToolTip(tr("Remove this subtitle track"));
    connect(action, &QAction::triggered, this, [&]() {
        show();
        raise();
        removeSubtitleTrack();
    });
    Actions.add("subtitleRemoveTrackAction", action);

    action = new QAction(tr("Edit Subtitle Track"), this);
    action->setIcon(QIcon::fromTheme("document-edit",
                                     QIcon(":/icons/oxygen/32x32/actions/document-edit.png")));
    action->setToolTip(tr("Edit this subtitle track"));
    connect(action, &QAction::triggered, this, [&]() {
        show();
        raise();
        editSubtitleTrack();
    });
    Actions.add("subtitleEditTrackAction", action);

    action = new QAction(tr("Import Subtitles From File"), this);
    action->setIcon(QIcon::fromTheme("document-import",
                                     QIcon(":/icons/oxygen/32x32/actions/document-import.png")));
    action->setToolTip(tr("Import subtitles from an srt file at the current position"));
    connect(action, &QAction::triggered, this, [&]() {
        show();
        raise();
        importSubtitles();
    });
    Actions.add("SubtitleImportAction", action);

    action = new QAction(tr("Export Subtitles To File"), this);
    action->setIcon(QIcon::fromTheme("document-export",
                                     QIcon(":/icons/oxygen/32x32/actions/document-export.png")));
    action->setToolTip(tr("Export the current subtitle track to an SRT file"));
    connect(action, &QAction::triggered, this, [&]() {
        show();
        raise();
        exportSubtitles();
    });
    Actions.add("SubtitleExportAction", action);

    action = new QAction(tr("Create/Edit Subtitle"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Q));
    action->setIcon(QIcon::fromTheme("subtitle",
                                     QIcon(":/icons/oxygen/32x32/actions/subtitle.png")));
    action->setToolTip(tr("Create or Edit a subtitle at the cursor position."));
    connect(action, &QAction::triggered, this, &SubtitlesDock::onCreateOrEditRequested);
    Actions.add("subtitleCreateEditItemAction", action, windowTitle());

    action = new QAction(tr("Add Subtitle Item"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_W));
    action->setIcon(QIcon::fromTheme("list-add",
                                     QIcon(":/icons/oxygen/32x32/actions/list-add.png")));
    action->setToolTip(tr("Add a subtitle at the cursor position"));
    connect(action, &QAction::triggered, this, &SubtitlesDock::onAddRequested);
    Actions.add("subtitleAddItemAction", action, windowTitle());

    action = new QAction(tr("Remove Subtitle Item"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_E));
    action->setIcon(QIcon::fromTheme("list-remove",
                                     QIcon(":/icons/oxygen/32x32/actions/list-remove.png")));
    action->setToolTip(tr("Remove the selected subtitle item"));
    connect(action, &QAction::triggered, this, &SubtitlesDock::onRemoveRequested);
    Actions.add("subtitleRemoveItemAction", action, windowTitle());

    action = new QAction(tr("Set Subtitle Start"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_R));
    action->setIcon(QIcon::fromTheme("keyframes-filter-in",
                                     QIcon(":/icons/oxygen/32x32/actions/keyframes-filter-in.png")));
    action->setToolTip(tr("Set the selected subtitle to start at the cursor position"));
    connect(action, &QAction::triggered, this, &SubtitlesDock::onSetStartRequested);
    Actions.add("subtitleSetStartAction", action, windowTitle());

    action = new QAction(tr("Set Subtitle End"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_T));
    action->setIcon(QIcon::fromTheme("keyframes-filter-out",
                                     QIcon(":/icons/oxygen/32x32/actions/keyframes-filter-out.png")));
    action->setToolTip(tr("Set the selected subtitle to end at the cursor position"));
    connect(action, &QAction::triggered, this, &SubtitlesDock::onSetEndRequested);
    Actions.add("subtitleSetEndAction", action, windowTitle());

    action = new QAction(tr("Move Subtitles"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Y));
    action->setIcon(QIcon::fromTheme("4-direction",
                                     QIcon(":/icons/oxygen/32x32/actions/4-direction.png")));
    action->setToolTip(tr("Move the selected subtitles to the cursor position"));
    connect(action, &QAction::triggered, this, &SubtitlesDock::onMoveRequested);
    Actions.add("subtitleMoveAction", action, windowTitle());

    action = new QAction(tr("Burn In Subtitles on Output"), this);
    action->setToolTip(
        tr("Create or edit a Burn In Subtitles filter on the timeline output."));
    action->setIcon(QIcon::fromTheme("font",
                                     QIcon(":/icons/oxygen/32x32/actions/font.png")));
    connect(action, &QAction::triggered, this, &SubtitlesDock::burnInOnTimeline);
    Actions.add("subtitleBurnInAction", action, windowTitle());

    action = new QAction(tr("Generate Text on Timeline"), this);
    action->setToolTip(
        tr("Create a new video track on the timeline with text showing these subtitles."));
    connect(action, &QAction::triggered, this, &SubtitlesDock::generateTextOnTimeline);
    Actions.add("subtitleGenerateTextAction", action, windowTitle());

    action = new QAction(tr("Speech to Text..."), this);
    action->setToolTip(
        tr("Detect speech and transcribe to a new subtitle track."));
    action->setIcon(QIcon::fromTheme("speech-to-text",
                                     QIcon(":/icons/oxygen/32x32/actions/speech-to-text.png")));
    connect(action, &QAction::triggered, this, &SubtitlesDock::speechToText);
    Actions.add("subtitleSpeechToTextAction", action, windowTitle());

    action = new QAction(tr("Track Timeline Cursor"), this);
    action->setToolTip(tr("Track the timeline cursor"));
    action->setCheckable(true);
    action->setChecked(Settings.subtitlesTrackTimeline());
    connect(action, &QAction::toggled, this, [&](bool checked) {
        Settings.setSubtitlesTrackTimeline(checked);
    });
    Actions.add("subtitleTrackTimelineAction", action, windowTitle());

    action = new QAction(tr("Show Previous/Next"), this);
    action->setToolTip(tr("Show the previous and next subtitles"));
    action->setCheckable(true);
    action->setChecked(Settings.subtitlesShowPrevNext());
    connect(action, &QAction::toggled, this, [&](bool checked) {
        Settings.setSubtitlesShowPrevNext(checked);
        resizeTextWidgets();
    });
    Actions.add("subtitleShowPrevNextAction", action, windowTitle());
}

void SubtitlesDock::setModel(SubtitlesModel *model, SubtitlesSelectionModel *selectionModel)
{
    m_model = model;
    m_selectionModel = selectionModel;
    m_treeView->setModel(m_model);
    m_treeView->setSelectionModel(m_selectionModel);
    m_treeView->header()->setStretchLastSection(false);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeView->setSelectionMode(QAbstractItemView::ContiguousSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setColumnHidden(2, !Settings.subtitlesShowColumn("start"));
    m_treeView->setColumnHidden(3, !Settings.subtitlesShowColumn("end"));
    m_treeView->setColumnHidden(4, !Settings.subtitlesShowColumn("duration"));
    connect(m_selectionModel, &QItemSelectionModel::currentChanged, this,
            &SubtitlesDock::refreshWidgets);
    connect(m_selectionModel, &SubtitlesSelectionModel::selectedTrackModelIndexChanged, m_treeView,
            &QTreeView::setRootIndex);
    connect(m_treeView, &QTreeView::doubleClicked, this, &SubtitlesDock::onItemDoubleClicked);
    connect(m_model, &QAbstractItemModel::modelReset, this, &SubtitlesDock::onModelReset);
    connect(m_model, &SubtitlesModel::tracksChanged, this, &SubtitlesDock::refreshTracksCombo);
    connect(m_model, &QAbstractItemModel::dataChanged, this, &SubtitlesDock::refreshWidgets);
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &SubtitlesDock::refreshWidgets);
    connect(m_model, &QAbstractItemModel::rowsMoved, this, &SubtitlesDock::refreshWidgets);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, [&](const QModelIndex & parent, int first,
    int last) {
        if (parent.isValid()) {
            refreshWidgets();
        }
    });
    refreshTracksCombo();
    refreshWidgets();
}

void SubtitlesDock::importSrtFromFile(const QString &srtPath, const QString &trackName,
                                      const QString &lang, bool includeNonspoken)
{
    QList<Subtitles::SubtitleItem> items = readSrtFile(srtPath, 0, includeNonspoken);
    if (items.size() == 0) {
        MAIN.showStatusMessage(QObject::tr("No subtitles found to import"));
        return;
    }

    SubtitlesModel::SubtitleTrack track;
    track.name = trackName;
    track.lang = lang;

    m_model->importSubtitlesToNewTrack(track, items);

    MAIN.showStatusMessage(QObject::tr("Imported %1 subtitle item(s)", nullptr, items.size()));
}

void SubtitlesDock::addSubtitleTrack()
{
    int newIndex = m_model->trackCount();
    if (!MAIN.isMultitrackValid()) {
        MAIN.showStatusMessage(tr("Add a clip to the timeline to create subtitles."));
        return;
    }
    QString currentLangCode = QLocale::languageToCode(QLocale::system().language(),
                                                      QLocale::ISO639Part2);
    SubtitleTrackDialog dialog(availableTrackName(), currentLangCode, this);
    if (dialog.exec() == QDialog::Accepted) {
        SubtitlesModel::SubtitleTrack track;
        track.name = dialog.getName();
        track.lang = dialog.getLanguage();
        if (trackNameExists(track.name)) {
            MAIN.showStatusMessage(tr("Subtitle track already exists: %1").arg(track.name));
            return;
        }
        m_model->addTrack(track);
    }
    m_trackCombo->setCurrentIndex(newIndex);
}

void SubtitlesDock::removeSubtitleTrack()
{
    QString name = m_trackCombo->currentData().toString();
    if (!name.isEmpty()) {
        Mlt::Producer *multitrack = MAIN.multitrack();
        if (multitrack && multitrack->is_valid()) {
            for (int i = 0; i < multitrack->filter_count(); i++) {
                QScopedPointer<Mlt::Filter> filter(multitrack->filter(i));
                if (!filter || !filter->is_valid() || filter->get("mlt_service") != QStringLiteral("subtitle")) {
                    continue;
                }
                if (name == filter->get("feed")) {
                    QMessageBox::warning(this, tr("Remove Subtitle Track"),
                                         tr("This track is in use by a subtitle filter.\n"
                                            "Remove the subtitle filter before removing this track."));
                    return;
                }
            }
        }
        m_model->removeTrack(name);
    }
}

void SubtitlesDock::editSubtitleTrack()
{
    if (m_model->trackCount() == 0) {
        addSubtitleTrack();
        return;
    }
    int currentIndex = m_selectionModel->selectedTrack();
    SubtitlesModel::SubtitleTrack track = m_model->getTrack(currentIndex);
    SubtitleTrackDialog dialog(track.name, track.lang, this);
    if (dialog.exec() == QDialog::Accepted) {
        QList<SubtitlesModel::SubtitleTrack> tracks = m_model->getTracks();
        for (int t = 0; t < tracks.count(); t++) {
            if (t == currentIndex) {
                if (dialog.getName() == track.name && dialog.getLanguage() == track.lang) {
                    // Nothing changed
                    return;
                }
            } else {
                if (dialog.getName() == track.name) {
                    MAIN.showStatusMessage(tr("Subtitle track already exists: %1").arg(track.name));
                    return;
                }
            }
        }
        track.name = dialog.getName();
        track.lang = dialog.getLanguage();
        m_model->editTrack(currentIndex, track);
    }
    m_trackCombo->setCurrentIndex(currentIndex);
}

void SubtitlesDock::importSubtitles()
{
    // Get the file name from the user.
    QString subtitlePath = QFileDialog::getOpenFileName(&MAIN, tr("Import Subtitle File"),
                                                        Settings.openPath(),
                                                        tr("Subtitle Files (*.srt *.SRT *.vtt *.VTT *.ass *.ASS *.ssa *.SSA)"),
                                                        nullptr, Util::getFileDialogOptions());
    if (subtitlePath.isEmpty()) {
        return;
    }
    QFileInfo subtitleFi(subtitlePath);
    if (!subtitleFi.exists()) {
        MAIN.showStatusMessage(tr("Unable to find subtitle file."));
        return;
    }

    MAIN.showStatusMessage(QObject::tr("Importing subtitles..."));

    // Convert the subtitles to SRT using FFMpeg
    QString tmpLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/";
    QScopedPointer<QTemporaryFile> tmp(Util::writableTemporaryFile(tmpLocation, "XXXXXX.srt"));
    tmp->open();
    QString tmpFileName = tmp->fileName();
    tmp->close();
    QProcess proc;
    QFileInfo ffmpegPath(qApp->applicationDirPath(), "ffmpeg");
    QStringList args;
    args << "-y" << "-hide_banner" << "-i" << subtitleFi.absoluteFilePath() << tmpFileName;
    LOG_INFO() << ffmpegPath.absoluteFilePath() << args;
    MAIN.showStatusMessage(QObject::tr("Importing subtitles..."));
    proc.setStandardOutputFile(QProcess::nullDevice());
    proc.setReadChannel(QProcess::StandardError);
    proc.start(ffmpegPath.absoluteFilePath(), args, QIODevice::ReadOnly);
    QCoreApplication::processEvents();
    if (!proc.waitForFinished(8000) || proc.exitStatus() != QProcess::NormalExit || proc.exitCode()) {
        QString output = proc.readAll();
        foreach (const QString &line, output.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts))
            LOG_INFO() << line;
    }

    // Read the subtitles
    int64_t msTime = positionToMs(m_pos);
    QList<Subtitles::SubtitleItem> items = readSrtFile(tmpFileName, msTime, true);
    if (items.size() == 0) {
        MAIN.showStatusMessage(QObject::tr("No subtitles found to import"));
        return;
    }
    ensureTrackExists();
    m_model->importSubtitles(m_trackCombo->currentIndex(), msTime, items);
    MAIN.showStatusMessage(QObject::tr("Imported %n subtitle item(s)", nullptr, items.size()));
}

void SubtitlesDock::exportSubtitles()
{
    auto track = m_model->getTrack(m_trackCombo->currentIndex());
    QString suggestedPath = QStringLiteral("%1/%2.srt").arg(Settings.savePath()).arg(track.name);
    QString srtPath = QFileDialog::getSaveFileName(&MAIN, tr("Export SRT File"), suggestedPath,
                                                   tr("SRT Files (*.srt *.SRT)"),
                                                   nullptr, Util::getFileDialogOptions());
    if (srtPath.isEmpty()) {
        return;
    }

    m_model->exportSubtitles(srtPath, m_trackCombo->currentIndex());
    Settings.setSavePath(srtPath);
}

void SubtitlesDock::onPositionChanged(int position)
{
    if (position == m_pos) {
        return;
    }
    m_pos = position;
    if (Actions["subtitleTrackTimelineAction"]->isChecked()) {
        selectItemForTime();
    }
}

void SubtitlesDock::refreshTracksCombo()
{
    if (m_model) {
        QList<SubtitlesModel::SubtitleTrack> tracks = m_model->getTracks();
        m_trackCombo->clear();
        for (auto &track : tracks) {
            m_trackCombo->addItem(QStringLiteral("%1 (%2)").arg(track.name).arg(track.lang), track.name);
        }
        if (tracks.size() > 0) {
            m_trackCombo->setCurrentIndex(0);
            m_selectionModel->setSelectedTrack(0);
        } else {
            m_trackCombo->setCurrentIndex(-1);
            m_selectionModel->setSelectedTrack(-1);
        }
        selectItemForTime();
    }
}

void SubtitlesDock::onCreateOrEditRequested()
{
    LOG_DEBUG();
    ensureTrackExists();
    int64_t msTime = positionToMs(m_pos);
    int trackIndex = m_trackCombo->currentIndex();
    int currentItemIndex = m_model->itemIndexAtTime(trackIndex, msTime);
    if (currentItemIndex >= 0) {
        setCurrentItem(trackIndex, currentItemIndex);
        m_text->setFocus();
        m_text->selectAll();
    } else {
        onAddRequested();
    }
}

void SubtitlesDock::onAddRequested()
{
    LOG_DEBUG();
    ensureTrackExists();
    int64_t msTime = positionToMs(m_pos);
    int trackIndex = m_trackCombo->currentIndex();
    if (m_model->itemIndexAtTime(trackIndex, msTime) >= 0) {
        MAIN.showStatusMessage(tr("A subtitle already exists at this time."));
        return;
    }
    int64_t maxTime = m_model->maxTime();
    int nextIndex = m_model->itemIndexAfterTime(trackIndex, msTime);
    if (nextIndex > -1) {
        auto nextItem = m_model->getItem(trackIndex, nextIndex);
        maxTime = nextItem.start;
    }
    if ((maxTime - msTime) < 500) {
        MAIN.showStatusMessage(tr("Not enough space to add subtitle."));
        return;
    }
    Subtitles::SubtitleItem item;
    item.start = msTime;
    item.end = msTime + DEFAULT_ITEM_DURATION;
    if (item.end > maxTime) {
        item.end = msTime + (maxTime - msTime);
    }
    m_model->overwriteItem(trackIndex, item);
    setCurrentItem(trackIndex, m_model->itemIndexAtTime(trackIndex, msTime));
    m_text->setFocus();
    m_text->selectAll();
}

void SubtitlesDock::onRemoveRequested()
{
    LOG_DEBUG();
    int trackIndex = m_trackCombo->currentIndex();
    QModelIndexList selectedRows = m_selectionModel->selectedRows();
    if (selectedRows.size() > 0) {
        int firstItemIndex = selectedRows[0].row();
        int lastItemIndex = selectedRows[selectedRows.size() - 1].row();
        m_model->removeItems(trackIndex, firstItemIndex, lastItemIndex);
    }
}

void SubtitlesDock::onSetStartRequested()
{
    LOG_DEBUG();
    int trackIndex = m_trackCombo->currentIndex();
    QModelIndexList selectedRows = m_selectionModel->selectedRows();
    if (selectedRows.size() > 0) {
        int itemIndex = selectedRows[0].row();
        int64_t msTime = positionToMs(m_pos);
        auto currentItem = m_model->getItem(trackIndex, itemIndex);
        if (msTime >= currentItem.end) {
            MAIN.showStatusMessage(tr("Start time can not be after end time."));
            return;
        }
        if (itemIndex > 0) {
            auto prevItem = m_model->getItem(trackIndex, itemIndex - 1);
            if (msTime < prevItem.end) {
                MAIN.showStatusMessage(tr("Start time can not be before previous subtitle."));
                return;
            }
        }
        m_model->setItemStart(trackIndex, itemIndex, msTime);
    }
}

void SubtitlesDock::onSetEndRequested()
{
    LOG_DEBUG();
    int trackIndex = m_trackCombo->currentIndex();
    QModelIndexList selectedRows = m_selectionModel->selectedRows();
    if (selectedRows.size() > 0) {
        int itemIndex = selectedRows[0].row();
        int64_t msTime = positionToMs(m_pos);
        auto currentItem = m_model->getItem(trackIndex, itemIndex);
        if (msTime <= currentItem.start) {
            MAIN.showStatusMessage(tr("End time can not be before start time."));
            return;
        }
        int itemCount = m_model->itemCount(trackIndex);
        if (itemIndex < (itemCount - 1)) {
            auto nextItem = m_model->getItem(trackIndex, itemIndex + 1);
            if (msTime > nextItem.start) {
                MAIN.showStatusMessage(tr("End time can not be after next subtitle."));
                return;
            }
        }
        m_model->setItemEnd(trackIndex, itemIndex, msTime);
    }
}

void SubtitlesDock::onMoveRequested()
{
    QModelIndexList selectedRows = m_selectionModel->selectedRows();
    if (selectedRows.size() <= 0) {
        return;
    }
    int64_t msTime = positionToMs(m_pos);
    if (m_model->validateMove(selectedRows, msTime)) {
        int trackIndex = selectedRows[0].parent().row();
        int firstItemIndex = selectedRows[0].row();
        int lastItemIndex = selectedRows[selectedRows.size() - 1].row();
        m_model->moveItems(trackIndex, firstItemIndex, lastItemIndex, msTime);
    } else {
        // This existing item will cause a conflict.
        MAIN.showStatusMessage(tr("Unable to move. Subtitles already exist at this time."));
    }
}

void SubtitlesDock::onTextEdited()
{
    LOG_DEBUG();
    QModelIndex currentIndex = m_selectionModel->currentIndex();
    if (currentIndex.isValid() && currentIndex.parent().isValid()) {
        m_textEditInProgress = true;
        m_model->setText(currentIndex.parent().row(), currentIndex.row(), m_text->toPlainText());
        m_textEditInProgress = false;
    } else {
        LOG_ERROR() << "Invalid index" << currentIndex;
    }
}

void SubtitlesDock::onModelReset()
{
    refreshTracksCombo();
    refreshWidgets();
}

void SubtitlesDock::onStartColumnToggled(bool checked)
{
    Settings.setSubtitlesShowColumn("start", checked);
    m_treeView->setColumnHidden(1, !checked);
}

void SubtitlesDock::onEndColumnToggled(bool checked)
{
    Settings.setSubtitlesShowColumn("end", checked);
    m_treeView->setColumnHidden(2, !checked);
}

void SubtitlesDock::onDurationColumnToggled(bool checked)
{
    Settings.setSubtitlesShowColumn("duration", checked);
    m_treeView->setColumnHidden(3, !checked);
}

void SubtitlesDock::onItemDoubleClicked(const QModelIndex &index)
{
    if (index.parent().isValid()) {
        const Subtitles::SubtitleItem item = m_model->getItem(index.parent().row(), index.row());
        int64_t position = msToPosition(item.start);
        emit seekRequested((int)position);
    }
}

void SubtitlesDock::resizeEvent(QResizeEvent *e)
{
    QDockWidget::resizeEvent(e);
    resizeTextWidgets();
}

void SubtitlesDock::resizeTextWidgets()
{
    if (Settings.subtitlesShowPrevNext()) {
        m_prev->setVisible(true);
        m_prev->setMinimumSize(QSize(width() / 3, m_text->maximumHeight()));
        m_text->setMinimumSize(QSize(width() / 3, m_text->maximumHeight()));
        m_next->setVisible(true);
        m_next->setMinimumSize(QSize(width() / 3, m_text->maximumHeight()));
        m_prevLabel->setVisible(true);
        m_textLabel->setVisible(true);
        m_nextLabel->setVisible(true);
    } else {
        m_text->setMinimumSize(QSize(width(), m_text->maximumHeight()));
        m_prev->setVisible(false);
        m_next->setVisible(false);
        m_prevLabel->setVisible(false);
        m_textLabel->setVisible(false);
        m_nextLabel->setVisible(false);
    }
}

void SubtitlesDock::updateTextWidgets()
{
    Subtitles::SubtitleItem item;
    const QSignalBlocker blocker(m_text);

    // First update based on current (selected) item.
    QModelIndex currentIndex = m_selectionModel->currentIndex();
    if (currentIndex.isValid() && currentIndex.parent().isValid()) {
        int trackIndex = currentIndex.parent().row();
        int itemIndex = currentIndex.row();
        if (trackIndex >= 0 && trackIndex < m_model->trackCount() && itemIndex >= 0
                && itemIndex < m_model->itemCount(trackIndex)) {
            item = m_model->getItem(trackIndex, itemIndex);
            m_text->setPlainText(QString::fromStdString(item.text));
            m_text->setReadOnly(false);
            if (itemIndex > 0) {
                item = m_model->getItem(trackIndex, itemIndex - 1);
                m_prev->setPlaceholderText(QString::fromStdString(item.text));
            } else {
                m_prev->setPlaceholderText(QString());
            }
            if (itemIndex < (m_model->itemCount(trackIndex) - 1)) {
                item = m_model->getItem(trackIndex, itemIndex + 1);
                m_next->setPlaceholderText(QString::fromStdString(item.text));
            } else {
                m_next->setPlaceholderText(QString());
            }
            return;
        } else {
            LOG_ERROR() << "Invalid row selected" << currentIndex;
        }
    }
    // If nothing selected, update based on position (but not editable)
    m_text->setReadOnly(true);
    int trackIndex = m_trackCombo->currentIndex();
    if (trackIndex >= 0 && m_pos >= 0) {
        int64_t msTime = positionToMs(m_pos);
        int itemIndex = m_model->itemIndexAtTime(trackIndex, msTime);
        if (itemIndex >= 0) {
            item = m_model->getItem(trackIndex, itemIndex);
            m_text->setPlainText(QString::fromStdString(item.text));
            m_text->setReadOnly(false);
            if (itemIndex > 0) {
                item = m_model->getItem(trackIndex, itemIndex - 1);
                m_prev->setPlaceholderText(QString::fromStdString(item.text));
            } else {
                m_prev->setPlaceholderText(QString());
            }
            if (itemIndex < (m_model->itemCount(trackIndex) - 1)) {
                item = m_model->getItem(trackIndex, itemIndex + 1);
                m_next->setPlaceholderText(QString::fromStdString(item.text));
            } else {
                m_next->setPlaceholderText(QString());
            }
        } else {
            m_text->clear();
            itemIndex = m_model->itemIndexBeforeTime(trackIndex, msTime);
            if (itemIndex >= 0) {
                item = m_model->getItem(trackIndex, itemIndex);
                m_prev->setPlaceholderText(QString::fromStdString(item.text));
            } else {
                m_prev->setPlaceholderText(QString());
            }
            itemIndex = m_model->itemIndexAfterTime(trackIndex, msTime);
            if (itemIndex >= 0) {
                item = m_model->getItem(trackIndex, itemIndex);
                m_next->setPlaceholderText(QString::fromStdString(item.text));
            } else {
                m_next->setPlaceholderText(QString());
            }
        }
    } else {
        m_text->setReadOnly(true);
        m_text->clear();
        m_prev->setPlaceholderText(QString());
        m_next->setPlaceholderText(QString());
    }
}

void SubtitlesDock::updateActionAvailablity()
{

    if (!m_model || !m_model->isValid()) {
        // Disable all actions
        m_addToTimelineLabel->setVisible(true);
        Actions["subtitleAddTrackAction"]->setEnabled(false);
        Actions["subtitleRemoveTrackAction"]->setEnabled(false);
        Actions["subtitleEditTrackAction"]->setEnabled(false);
        Actions["SubtitleImportAction"]->setEnabled(false);
        Actions["SubtitleExportAction"]->setEnabled(false);
        Actions["subtitleCreateEditItemAction"]->setEnabled(false);
        Actions["subtitleAddItemAction"]->setEnabled(false);
        Actions["subtitleRemoveItemAction"]->setEnabled(false);
        Actions["subtitleMoveAction"]->setEnabled(false);
        Actions["subtitleSetStartAction"]->setEnabled(false);
        Actions["subtitleSetEndAction"]->setEnabled(false);
        Actions["subtitleBurnInAction"]->setEnabled(false);
        Actions["subtitleGenerateTextAction"]->setEnabled(false);
        Actions["subtitleSpeechToTextAction"]->setEnabled(false);
    } else {
        m_addToTimelineLabel->setVisible(false);
        Actions["subtitleCreateEditItemAction"]->setEnabled(true);
        Actions["subtitleAddTrackAction"]->setEnabled(true);
        Actions["subtitleEditTrackAction"]->setEnabled(true);
        Actions["SubtitleImportAction"]->setEnabled(true);
        Actions["subtitleAddItemAction"]->setEnabled(true);
        Actions["subtitleSpeechToTextAction"]->setEnabled(true);
        if (m_model->trackCount() == 0) {
            Actions["subtitleRemoveTrackAction"]->setEnabled(false);
            Actions["SubtitleExportAction"]->setEnabled(false);
            Actions["subtitleRemoveItemAction"]->setEnabled(false);
            Actions["subtitleMoveAction"]->setEnabled(false);
            Actions["subtitleSetStartAction"]->setEnabled(false);
            Actions["subtitleSetEndAction"]->setEnabled(false);
            Actions["subtitleBurnInAction"]->setEnabled(false);
            Actions["subtitleGenerateTextAction"]->setEnabled(false);
        } else {
            Actions["subtitleRemoveTrackAction"]->setEnabled(true);
            Actions["SubtitleExportAction"]->setEnabled(true);
            Actions["subtitleBurnInAction"]->setEnabled(true);
            Actions["subtitleGenerateTextAction"]->setEnabled(true);

            if (m_selectionModel->selectedRows().size() == 1) {
                Actions["subtitleSetStartAction"]->setEnabled(true);
                Actions["subtitleSetEndAction"]->setEnabled(true);
            } else {
                Actions["subtitleSetStartAction"]->setEnabled(false);
                Actions["subtitleSetEndAction"]->setEnabled(false);
            }
            if (m_selectionModel->selectedRows().size() > 0) {
                Actions["subtitleRemoveItemAction"]->setEnabled(true);
                Actions["subtitleMoveAction"]->setEnabled(true);
            } else {
                Actions["subtitleRemoveItemAction"]->setEnabled(false);
                Actions["subtitleMoveAction"]->setEnabled(false);
            }
        }
    }
}

void SubtitlesDock::refreshWidgets()
{
    if (m_textEditInProgress) {
        return;
    }
    updateTextWidgets();
    updateActionAvailablity();
}

void SubtitlesDock::setCurrentItem(int trackIndex, int itemIndex)
{
    if (itemIndex >= 0) {
        QModelIndex modelIndex = m_model->itemModelIndex(trackIndex, itemIndex);
        if (modelIndex.isValid()) {
            m_treeView->setCurrentIndex(modelIndex);
            return;
        }
    }
    m_treeView->setCurrentIndex(QModelIndex());
}

void SubtitlesDock::selectItemForTime()
{
    int trackIndex = m_trackCombo->currentIndex();
    if (trackIndex >= 0) {
        int64_t time = positionToMs(m_pos);
        int itemIndex = m_model->itemIndexAtTime(trackIndex, time);
        setCurrentItem(trackIndex, itemIndex);
    }
}

QString SubtitlesDock::availableTrackName()
{
    int i = 1;
    QString suggestedName = tr("Subtitle Track %1").arg(i);
    while (trackNameExists(suggestedName)) {
        suggestedName = tr("Subtitle Track %1").arg(i++);
    }
    return suggestedName;
}

bool SubtitlesDock::trackNameExists(const QString &name)
{
    QList<SubtitlesModel::SubtitleTrack> tracks = m_model->getTracks();
    for (auto &track : tracks) {
        if (track.name == name) {
            return true;
        }
    }
    return false;
}

void SubtitlesDock::ensureTrackExists()
{
    if (m_model->trackCount() == 0) {
        SubtitlesModel::SubtitleTrack track;
        track.name = tr("Subtitle Track %1").arg(1);
        track.lang = QLocale::languageToCode(QLocale::system().language(), QLocale::ISO639Part2);
        m_model->addTrack(track);
    }
}

void SubtitlesDock::burnInOnTimeline()
{
    int trackIndex = m_trackCombo->currentIndex();
    auto track = m_model->getTrack(trackIndex);
    Mlt::Filter filter(MLT.profile(), "subtitle");
    filter.set(kShotcutFilterProperty, "subtitles");
#if defined(Q_OS_WIN)
    filter.set("family", "Verdana");
#elif defined(Q_OS_MAC)
    filter.set("family", "Helvetica");
#endif
    filter.set("fgcolour", "#ffffffff");
    filter.set("bgcolour", "#00000000");
    filter.set("olcolour", "#aa000000");
    filter.set("outline", 3);
    filter.set("weight", QFont::Bold);
    filter.set("style", "normal");
    filter.set("shotcut:usePointSize", 1);
    filter.set("size", MLT.profile().height() / 20);
    filter.set("geometry", "20%/75%:60%x20%");
    filter.set("valign", "bottom");
    filter.set("halign", "center");
    filter.set("feed", track.name.toUtf8().constData());
    emit createOrEditFilterOnOutput(&filter, QStringList() << "feed");
}

void SubtitlesDock::generateTextOnTimeline()
{
    int trackIndex = m_trackCombo->currentIndex();
    int itemCount = m_model->itemCount(trackIndex);
    if (itemCount == 0) {
        return;
    }
    Mlt::Playlist playlist(MLT.profile());
    int lastItemFrameEnd = 0;
    for (int itemIndex = 0; itemIndex < itemCount; itemIndex++) {
        auto item = m_model->getItem(trackIndex, itemIndex);
        int frameStart = msToPosition(item.start);
        int frameEnd = msToPosition(item.end);
        // Create a transparent color producer
        Mlt::Producer producer(MLT.profile(), "color:");
        producer.set("resource", "#00000000");
        producer.set("mlt_image_format", "rgba");
        producer.set("in", 0);
        producer.set("out", frameEnd - frameStart);
        // Add a text filter
        Mlt::Filter filter(MLT.profile(), "dynamictext");
        filter.set(kShotcutFilterProperty, "dynamicText");
        filter.set("argument", item.text.c_str());
#ifdef Q_OS_WIN
        filter.set("family", "Verdana");
#elif defined(Q_OS_MAC)
        filter.set("family", "Helvetica");
#endif
        filter.set("fgcolour", "#ffffffff");
        filter.set("bgcolour", "#00000000");
        filter.set("olcolour", "#aa000000");
        filter.set("outline", 3);
        filter.set("weight", QFont::Bold);
        filter.set("style", "normal");
        filter.set("shotcut:usePointSize", 1);
        filter.set("size", MLT.profile().height() / 20);
        filter.set("geometry", "20%/75%:60%x20%");
        filter.set("valign", "bottom");
        filter.set("halign", "center");
        filter.set_in_and_out(producer.get_in(), producer.get_out());
        producer.attach(filter);
        if (lastItemFrameEnd < (frameStart - 1)) {
            playlist.blank(frameStart - lastItemFrameEnd - 1);
        }
        playlist.append(producer, 0, frameEnd - frameStart - 1);
        lastItemFrameEnd = frameEnd;
    }
    emit addAllTimeline(&playlist, true, true);
}

void SubtitlesDock::speechToText()
{
    TranscribeAudioDialog dialog(availableTrackName(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    if (trackNameExists(dialog.name())) {
        MAIN.showStatusMessage(tr("Subtitle track already exists: %1").arg(dialog.name()));
        return;
    }

    QString tmpLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/";

    // Make a copy of the timeline producer
    QString xml = MLT.XML(MAIN.multitrack());
    std::unique_ptr<Mlt::Producer> tempProducer(new Mlt::Producer(MLT.profile(), "xml-string",
                                                                  xml.toUtf8().constData()));

    // Mute tracks as requested
    Mlt::Tractor tractor(*tempProducer.get());
    if (!tractor.is_valid()) {
        LOG_ERROR() << "Invalid tractor";
        return;
    }
    int trackCount = tractor.count();
    if (trackCount <= 0) {
        LOG_ERROR() << "No tracks";
        return;
    }
    QList<int> tracks = dialog.tracks();
    for (int trackIndex = 0; trackIndex < trackCount; trackIndex++) {
        std::unique_ptr<Mlt::Producer> track(tractor.track(trackIndex));
        if (track) {
            if (tracks.contains(trackIndex)) {
                track->set("hide", 0);
            } else {
                track->set("hide", 2);
            }
        }
    }

    // Convert to XML
    QString wavXml = MLT.XML(tempProducer.get());

    // Create a temporary wav file
    QTemporaryFile *tmpWav = Util::writableTemporaryFile(tmpLocation, "shotcut-XXXXXX.wav");
    if (!tmpWav->open()) {
        LOG_ERROR() << "Failed to open temporary file" << tmpWav->fileName();
        return;
    }
    tmpWav->close();

    // Make the wav file from the timeline
    QStringList args;
    args << "-consumer" << "avformat:" + tmpWav->fileName() << "video_off=1" << "ar=16000" << "ac=1";
    QString jobName = tr("Extracting Audio");
    MeltJob *wavJob = new MeltJob(jobName, wavXml, args, MLT.profile().frame_rate_num(),
                                  MLT.profile().frame_rate_den());
    tmpWav->setParent(wavJob);
    JOBS.add(wavJob);

    // Create a temporary srt file
    QTemporaryFile *tmpSrt = Util::writableTemporaryFile(tmpLocation, "shotcut-XXXXXX.srt");
    if (!tmpSrt->open()) {
        LOG_ERROR() << "Failed to open temporary file" << tmpSrt->fileName();
        return;
    }
    tmpWav->close();

    // Run speech transcription on the wav file
    jobName = tr("Speech to Text");
    WhisperJob *whisperJob = new WhisperJob(jobName, tmpWav->fileName(), tmpSrt->fileName(),
                                            dialog.language(), dialog.translate(), dialog.maxLineLength());
    // Ensure the language code is 3 character (part 2)
    QString langCode = dialog.language();
    QLocale::Language lang = QLocale::codeToLanguage(langCode);
    if (lang != QLocale::AnyLanguage) {
        langCode = QLocale::languageToCode(lang, QLocale::ISO639Part2);
    }
    whisperJob->setPostJobAction(new ImportSrtPostJobAction(tmpSrt->fileName(), dialog.name(), langCode,
                                                            dialog.includeNonspoken(), this));
    tmpSrt->setParent(whisperJob);
    JOBS.add(whisperJob);
}
