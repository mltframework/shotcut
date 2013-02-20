/*
 * Copyright (c) 2012 Meltytech, LLC
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

#include "playlistdock.h"
#include "ui_playlistdock.h"
#include "dialogs/durationdialog.h"
#include "mainwindow.h"
#include <QtGui/QMenu>
#include <QDebug>

PlaylistDock::PlaylistDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PlaylistDock)
{
    ui->setupUi(this);
    toggleViewAction()->setIcon(QIcon::fromTheme("view-media-playlist", windowIcon()));
    ui->tableView->setModel(&m_model);
    ui->tableView->setDragDropMode(QAbstractItemView::DragDrop);
    ui->tableView->setDropIndicatorShown(true);
    ui->tableView->setDragDropOverwriteMode(false);
    ui->tableView->setAcceptDrops(true);
    ui->tableView->setDefaultDropAction(Qt::MoveAction);
    ui->tableView->resizeColumnToContents(0);
    connect(ui->actionRemove, SIGNAL(triggered()), this, SLOT(on_removeButton_clicked()));
    connect(&m_model, SIGNAL(cleared()), this, SLOT(onPlaylistCleared()));
    connect(&m_model, SIGNAL(created()), this, SLOT(onPlaylistCreated()));
    connect(&m_model, SIGNAL(loaded()), this, SLOT(onPlaylistLoaded()));
    connect(&m_model, SIGNAL(modified()), this, SLOT(onPlaylistCreated()));
    connect(&m_model, SIGNAL(dropped(const QMimeData*,int)), this, SLOT(onDropped(const QMimeData*,int)));
    connect(&m_model, SIGNAL(moveClip(int,int)), SLOT(onMoveClip(int,int)));
}

PlaylistDock::~PlaylistDock()
{
    delete ui;
}

int PlaylistDock::position()
{
    int result = -1;
    QModelIndex index = ui->tableView->currentIndex();
    if (index.isValid()) {
        Mlt::ClipInfo* i = m_model.playlist()->clip_info(index.row());
        if (i) result = i->start;
    }
    return result;
}

void PlaylistDock::incrementIndex()
{
    QModelIndex index = ui->tableView->currentIndex();
    if (!index.isValid())
        index = m_model.createIndex(0, 0);
    else
        index = m_model.incrementIndex(index);
    if (index.isValid())
        ui->tableView->setCurrentIndex(index);
}

void PlaylistDock::decrementIndex()
{
    QModelIndex index = ui->tableView->currentIndex();
    if (!index.isValid())
        index = m_model.createIndex(0, 0);
    else
        index = m_model.decrementIndex(index);
    if (index.isValid())
        ui->tableView->setCurrentIndex(index);
}

void PlaylistDock::setIndex(int row)
{
    QModelIndex index = m_model.createIndex(row, 0);
    if (index.isValid())
        ui->tableView->setCurrentIndex(index);
}

void PlaylistDock::moveClipUp()
{
    int row = ui->tableView->currentIndex().row();
    if (row > 0)
        MAIN.undoStack()->push(new Playlist::MoveCommand(m_model, row, row - 1));
}

void PlaylistDock::moveClipDown()
{
    int row = ui->tableView->currentIndex().row();
    if (row + 1 < m_model.rowCount())
        MAIN.undoStack()->push(new Playlist::MoveCommand(m_model, row, row + 1));
}

void PlaylistDock::on_menuButton_clicked()
{
    QPoint pos = ui->menuButton->mapToParent(QPoint(0, 0));
    QMenu menu(this);
    QModelIndex index = ui->tableView->currentIndex();
    if (index.isValid()) {
        menu.addAction(ui->actionGoto);
        if (MLT.producer()->type() != playlist_type)
            menu.addAction(ui->actionInsertCut);
        menu.addAction(ui->actionOpen);

        Mlt::ClipInfo* info = m_model.playlist()->clip_info(index.row());
        if (info && info->resource && MLT.producer()->get("resource")
                && !strcmp(info->resource, MLT.producer()->get("resource"))) {
            menu.addAction(ui->actionUpdate);
        }
        delete info;

        menu.addAction(ui->actionRemove);
        menu.addSeparator();
    }
    menu.addAction(ui->actionRemoveAll);
    menu.addAction(ui->actionClose);
    menu.exec(mapToGlobal(pos));
}

void PlaylistDock::on_actionInsertCut_triggered()
{
    onDropped(0, ui->tableView->currentIndex().row());
}

void PlaylistDock::on_actionAppendCut_triggered()
{
    if (MLT.producer() && MLT.producer()->is_valid()) {
        if (MLT.producer()->type() == playlist_type)
            emit showStatusMessage(tr("You cannot insert a playlist into a playlist!"));
        else if (MLT.isSeekable())
            MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.saveXML("string")));
        else {
            DurationDialog dialog(this);
            dialog.setDuration(MLT.profile().fps() * 5);
            if (dialog.exec() == QDialog::Accepted) {
                MLT.producer()->set_in_and_out(0, dialog.duration() - 1);
                if (MLT.producer()->get("mlt_service") && !strcmp(MLT.producer()->get("mlt_service"), "avformat"))
                    MLT.producer()->set("mlt_service", "avformat-novalidate");
                MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.saveXML("string")));
            }
        }
    }
    else {
        MAIN.openVideo();
    }
}

void PlaylistDock::on_actionInsertBlank_triggered()
{
    DurationDialog dialog(this);
    dialog.setDuration(MLT.profile().fps() * 5);
    if (dialog.exec() == QDialog::Accepted) {
        QModelIndex index = ui->tableView->currentIndex();
        if (index.isValid())
            m_model.insertBlank(dialog.duration(), index.row());
        else
            m_model.appendBlank(dialog.duration());
    }
}

void PlaylistDock::on_actionAppendBlank_triggered()
{
    DurationDialog dialog(this);
    dialog.setDuration(MLT.profile().fps() * 5);
    if (dialog.exec() == QDialog::Accepted)
        m_model.appendBlank(dialog.duration());
}

void PlaylistDock::on_actionUpdate_triggered()
{
    QModelIndex index = ui->tableView->currentIndex();
    if (!index.isValid()) return;
    Mlt::ClipInfo* info = m_model.playlist()->clip_info(index.row());
    if (!info) return;
    if (info->resource && MLT.producer()->get("resource")
            && !strcmp(info->resource, MLT.producer()->get("resource"))) {
        if (MLT.isSeekable()) {
            MAIN.undoStack()->push(new Playlist::UpdateCommand(m_model, MLT.saveXML("string"), index.row()));
        }
        else {
            // change the duration
            DurationDialog dialog(this);
            dialog.setDuration(info->frame_count);
            if (dialog.exec() == QDialog::Accepted) {
                MLT.producer()->set_in_and_out(0, dialog.duration() - 1);
                if (MLT.producer()->get("mlt_service") && !strcmp(MLT.producer()->get("mlt_service"), "avformat"))
                    MLT.producer()->set("mlt_service", "avformat-novalidate");
                MAIN.undoStack()->push(new Playlist::UpdateCommand(m_model, MLT.saveXML("string"), index.row()));
            }
        }
    }
    else {
        emit showStatusMessage(tr("This clip does not match the selected cut in the playlist!"));
    }
    delete info;
}

void PlaylistDock::on_removeButton_clicked()
{
    QModelIndex index = ui->tableView->currentIndex();
    if (!index.isValid()) return;
    MAIN.undoStack()->push(new Playlist::RemoveCommand(m_model, index.row()));
    int count = m_model.playlist()->count();
    if (count == 0) return;
    Mlt::ClipInfo* i = m_model.playlist()->clip_info(
                index.row() >= count? count-1 : index.row());
    if (i) {
        emit itemActivated(i->start);
        delete i;
    }
}

void PlaylistDock::on_actionOpen_triggered()
{
    QModelIndex index = ui->tableView->currentIndex();
    if (!index.isValid()) return;
    Mlt::ClipInfo* i = m_model.playlist()->clip_info(index.row());
    if (i) {
        Mlt::Producer* p = new Mlt::Producer(i->producer);
        emit clipOpened(p, i->frame_in, i->frame_out);
        delete i;
    }
}

void PlaylistDock::on_tableView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableView->currentIndex();
    if (index.isValid()) {
        QMenu menu(this);
        menu.addAction(ui->actionGoto);
        if (MLT.producer()->type() != playlist_type)
            menu.addAction(ui->actionInsertCut);
        menu.addAction(ui->actionOpen);

        Mlt::ClipInfo* info = m_model.playlist()->clip_info(index.row());
        if (info && info->resource && MLT.producer()->get("resource")
                && !strcmp(info->resource, MLT.producer()->get("resource"))) {
            menu.addAction(ui->actionUpdate);
        }
        delete info;

        menu.addAction(ui->actionRemove);
        menu.exec(mapToGlobal(pos));
    }
}

void PlaylistDock::on_tableView_doubleClicked(const QModelIndex &index)
{
    Mlt::ClipInfo* i = m_model.playlist()->clip_info(index.row());
    if (i) {
        emit itemActivated(i->start);
        delete i;
    }
}

void PlaylistDock::on_actionGoto_triggered()
{
    on_tableView_doubleClicked(ui->tableView->currentIndex());
}

void PlaylistDock::on_actionRemoveAll_triggered()
{
    MAIN.undoStack()->push(new Playlist::ClearCommand(m_model));
}

void PlaylistDock::on_actionClose_triggered()
{
    if (MAIN.continueModified())
        m_model.close();
}

void PlaylistDock::onPlaylistCreated()
{
    ui->removeButton->setEnabled(true);
    ui->menuButton->setEnabled(true);
    ui->stackedWidget->setCurrentIndex(1);
}

void PlaylistDock::onPlaylistLoaded()
{
    onPlaylistCreated();
    ui->tableView->resizeColumnsToContents();
}

void PlaylistDock::onPlaylistCleared()
{
    ui->removeButton->setEnabled(false);
    ui->menuButton->setEnabled(false);
    ui->stackedWidget->setCurrentIndex(0);
}

void PlaylistDock::onDropped(const QMimeData *data, int row)
{
    if (data && data->hasUrls()) {
        foreach (QUrl url, data->urls()) {
            Mlt::Producer p(MLT.profile(), url.path().toUtf8().constData());
            if (p.is_valid()) {
                if (row == -1)
                    MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.saveXML("string", &p)));
                else
                    MAIN.undoStack()->push(new Playlist::InsertCommand(m_model, MLT.saveXML("string", &p), row));
            }
        }
    }
    else if (!data || data->data("application/mlt+xml").isEmpty()) {
        if (MLT.producer() && MLT.producer()->is_valid()) {
            if (MLT.producer()->type() == playlist_type)
                emit showStatusMessage(tr("You cannot insert a playlist into a playlist!"));
            else if (MLT.isSeekable()) {
                if (row == -1)
                    MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.saveXML("string")));
                else
                    MAIN.undoStack()->push(new Playlist::InsertCommand(m_model, MLT.saveXML("string"), row));
            } else {
                DurationDialog dialog(this);
                dialog.setDuration(MLT.profile().fps() * 5);
                if (dialog.exec() == QDialog::Accepted) {
                    MLT.producer()->set_in_and_out(0, dialog.duration() - 1);
                    if (MLT.producer()->get("mlt_service") && !strcmp(MLT.producer()->get("mlt_service"), "avformat"))
                        MLT.producer()->set("mlt_service", "avformat-novalidate");
                    if (row == -1)
                        MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.saveXML("string")));
                    else
                        MAIN.undoStack()->push(new Playlist::InsertCommand(m_model, MLT.saveXML("string"), row));
                }
            }
        }
    }
}

void PlaylistDock::onMoveClip(int from, int to)
{
    MAIN.undoStack()->push(new Playlist::MoveCommand(m_model, from, to));
}

void PlaylistDock::onPlayerDragStarted()
{
    if (isVisible())
        ui->stackedWidget->setCurrentIndex(1);
}

void PlaylistDock::on_addButton_clicked()
{
    on_actionAppendCut_triggered();
}

namespace Playlist
{

AppendCommand::AppendCommand(PlaylistModel& model, const QString& xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_xml(xml)
{
    setText(QObject::tr("Append playlist item %1").arg(m_model.rowCount() + 1));
}

void AppendCommand::redo()
{
    Mlt::Producer* producer = new Mlt::Producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.append(producer);
    delete producer;
}

void AppendCommand::undo()
{
    m_model.remove(m_model.rowCount() - 1);
}

InsertCommand::InsertCommand(PlaylistModel& model, const QString& xml, int row, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_xml(xml)
    , m_row(row)
{
    setText(QObject::tr("Insert playist item %1").arg(row + 1));
}

void InsertCommand::redo()
{
    Mlt::Producer* producer = new Mlt::Producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.insert(producer, m_row);
    delete producer;
}

void InsertCommand::undo()
{
    m_model.remove(m_row);
}

UpdateCommand::UpdateCommand(PlaylistModel& model, const QString& xml, int row, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_newXml(xml)
    , m_row(row)
{
    setText(QObject::tr("Update playlist item %1").arg(row + 1));
    m_oldXml = MLT.saveXML("string", m_model.playlist()->get_clip(m_row));
}

void UpdateCommand::redo()
{
    Mlt::Producer* producer = new Mlt::Producer(MLT.profile(), "xml-string", m_newXml.toUtf8().constData());
    m_model.update(m_row, producer);
    delete producer;
}

void UpdateCommand::undo()
{
    Mlt::Producer* producer = new Mlt::Producer(MLT.profile(), "xml-string", m_oldXml.toUtf8().constData());
    m_model.update(m_row, producer);
    delete producer;
}

RemoveCommand::RemoveCommand(PlaylistModel& model, int row, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_row(row)
{
    m_xml = MLT.saveXML("string", m_model.playlist()->get_clip(m_row));
    setText(QObject::tr("Remove playlist item %1").arg(row + 1));
}

void RemoveCommand::redo()
{
    m_model.remove(m_row);
}

void RemoveCommand::undo()
{
    Mlt::Producer* producer = new Mlt::Producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.insert(producer, m_row);
    delete producer;
}

ClearCommand::ClearCommand(PlaylistModel& model, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
{
    m_xml = MLT.saveXML("string", m_model.playlist());
    setText(QObject::tr("Clear playlist"));
}

void ClearCommand::redo()
{
    m_model.clear();
}

void ClearCommand::undo()
{
    m_model.close();
    Mlt::Producer* producer = new Mlt::Producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    if (producer->is_valid()) {
        producer->set("resource", "<playlist>");
        MAIN.open(producer);
        MLT.pause();
        MAIN.seekPlaylist(0);
    }
}

MoveCommand::MoveCommand(PlaylistModel &model, int from, int to, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_from(from)
    , m_to(to)
{
    setText(QObject::tr("Move item from %1 to %2").arg(from + 1).arg(to + 1));
}

void MoveCommand::redo()
{
    m_model.move(m_from, m_to);
}

void MoveCommand::undo()
{
    m_model.move(m_to, m_from);
}

} // namespace Playlist
