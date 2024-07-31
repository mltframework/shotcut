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

#ifndef SUBTITLESDOCK_H
#define SUBTITLESDOCK_H

#include "MltPlaylist.h"

#include <QDockWidget>

class SubtitlesModel;
class SubtitlesSelectionModel;
class QComboBox;
class QItemSelection;
class QLabel;
class QLineEdit;
class QTextEdit;
class QToolButton;
class QTreeView;

class SubtitlesDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit SubtitlesDock(QWidget *parent = 0);
    ~SubtitlesDock();
    void setModel(SubtitlesModel *model, SubtitlesSelectionModel *selectionModel);

signals:
    void seekRequested(int pos);
    void addAllTimeline(Mlt::Playlist *, bool skipProxy, bool emptyTrack);

private slots:
    void onPositionChanged(int position);
    void onStartColumnToggled(bool checked);
    void onEndColumnToggled(bool checked);
    void onDurationColumnToggled(bool checked);

protected:
    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;

private:
    void setupActions();
    void onCreateOrEditRequested();
    void onAddRequested();
    void onRemoveRequested();
    void onSetStartRequested();
    void onSetEndRequested();
    void onMoveRequested();
    void onTextEdited();
    void onModelReset();
    void updateActionAvailablity();
    void addSubtitleTrack();
    void removeSubtitleTrack();
    void editSubtitleTrack();
    void refreshTracksCombo();
    void importSubtitles();
    void exportSubtitles();
    void onItemDoubleClicked(const QModelIndex &index);
    void resizeTextWidgets();
    void updateTextWidgets();
    void setCurrentItem(int trackIndex, int itemIndex);
    void refreshWidgets();
    void selectItemForTime();
    bool trackNameExists(const QString &name);
    void ensureTrackExists();
    void generateTextOnTimeline();

    SubtitlesModel *m_model;
    SubtitlesSelectionModel *m_selectionModel;
    QLabel *m_addToTimelineLabel;
    QComboBox *m_trackCombo;
    QTreeView *m_treeView;
    QTextEdit *m_text;
    QTextEdit *m_prev;
    QTextEdit *m_next;
    QLabel *m_prevLabel;
    QLabel *m_textLabel;
    QLabel *m_nextLabel;
    int m_pos;
    bool m_textEditInProgress;
};

#endif // SUBTITLESDOCK_H
