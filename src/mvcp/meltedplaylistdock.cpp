/*
 * Copyright (c) 2012-2015 Meltytech, LLC
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

#include "meltedplaylistdock.h"
#include "ui_meltedplaylistdock.h"
#include "mainwindow.h"
#include "qmltypes/qmlapplication.h"

#include <QMimeData>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>

MeltedPlaylistDock::MeltedPlaylistDock(QWidget *parent)
    : QDockWidget(parent)
    , ui(new Ui::MeltedPlaylistDock)
    , m_transportControl(new MeltedPlaylist::TransportControl(m_model))
{
    ui->setupUi(this);
    ui->tableView->setModel(&m_model);
    ui->tableView->setDragDropMode(QAbstractItemView::DragDrop);
    ui->tableView->setDropIndicatorShown(true);
    ui->tableView->setDragDropOverwriteMode(false);
    ui->tableView->setAcceptDrops(true);
    connect(&m_model, SIGNAL(loaded()), ui->tableView, SLOT(resizeColumnsToContents()));
    connect(&m_model, SIGNAL(dropped(QString,int)), this, SLOT(onDropped(QString,int)));
    connect(&m_model, SIGNAL(moveClip(int,int)), this, SLOT(onMoveClip(int,int)));
    connect(&m_model, SIGNAL(success()), this, SLOT(onSuccess()));
    connect(ui->actionAppendCut, SIGNAL(triggered()), this, SLOT(on_addButton_clicked()));
    connect(ui->actionRemove, SIGNAL(triggered()), this, SLOT(on_removeButton_clicked()));
}

MeltedPlaylistDock::~MeltedPlaylistDock()
{
    delete m_transportControl;
    delete ui;
}

QAbstractItemModel *MeltedPlaylistDock::model() const
{
    return (QAbstractItemModel*) &m_model;
}

const TransportControllable *MeltedPlaylistDock::transportControl() const
{
    return m_transportControl;
}

void MeltedPlaylistDock::onConnected(const QString &address, quint16 port, quint8 unit)
{
    m_model.onConnected(address, port, unit);
}

void MeltedPlaylistDock::onDisconnected()
{
    m_model.onDisconnected();
}

void MeltedPlaylistDock::onUnitChanged(quint8 unit)
{
    m_model.onUnitChanged(unit);
}

void MeltedPlaylistDock::keyPressEvent(QKeyEvent *ev)
{
    if (ui->tableView->currentIndex().isValid())
    if (ev->key() == Qt::Key_Backspace || ev->key() == Qt::Key_Delete)
        on_removeButton_clicked();
}

void MeltedPlaylistDock::on_tableView_clicked(const QModelIndex &index)
{
    Q_UNUSED(index)
    ui->removeButton->setEnabled(true);
}

void MeltedPlaylistDock::on_tableView_doubleClicked(const QModelIndex &index)
{
    m_model.gotoClip(index.row());
}

void MeltedPlaylistDock::onDropped(QString clip, int row)
{
    if (row == -1)
        onAppend(clip);
    else
        onInsert(clip, row);
}

void MeltedPlaylistDock::onAppend(QString clip, int in, int out)
{
    m_operations.append(new MeltedPlaylist::AppendCommand(m_model, clip));
    m_model.append(clip, in, out);
}

void MeltedPlaylistDock::onInsert(QString clip, int row, int in, int out)
{
    m_operations.append(new MeltedPlaylist::InsertCommand(m_model, clip, row));
    m_model.insert(clip, row, in, out);
}

void MeltedPlaylistDock::onMoveClip(int from, int to)
{
    QString clip = m_model.data(m_model.index(from, MeltedPlaylistModel::COLUMN_RESOURCE), Qt::ToolTipRole).toString();
    m_operations.append(new MeltedPlaylist::MoveCommand(m_model, clip, from, to));
    m_model.move(from, to);
}

void MeltedPlaylistDock::onSuccess()
{
    if (!m_operations.isEmpty())
        MAIN.undoStack()->push(m_operations.takeFirst());
}

void MeltedPlaylistDock::on_tableView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableView->currentIndex();
    if (index.isValid()) {
        QMenu menu(this);
        menu.addAction(ui->actionGoto);
        menu.addAction(ui->actionInsertCut);
//        menu.addAction(ui->actionOpen);
        menu.addAction(ui->actionRemove);
        menu.exec(mapToGlobal(pos));
    }
}

void MeltedPlaylistDock::on_addButton_clicked()
{
    emit appendRequested();
}

void MeltedPlaylistDock::on_removeButton_clicked()
{
    int row = ui->tableView->currentIndex().row();
    QString clip = m_model.data(m_model.index(row, MeltedPlaylistModel::COLUMN_RESOURCE), Qt::ToolTipRole).toString();
    int in = m_model.data(m_model.index(row, MeltedPlaylistModel::COLUMN_IN), Qt::DisplayRole).toInt();
    int out = m_model.data(m_model.index(row, MeltedPlaylistModel::COLUMN_OUT), Qt::DisplayRole).toInt();
    m_operations.append(new MeltedPlaylist::RemoveCommand(m_model, clip, row, in, out));
    m_model.remove(row);
}

void MeltedPlaylistDock::on_menuButton_clicked()
{
    QPoint pos = ui->menuButton->mapToParent(QPoint(0, 0));
    QMenu menu(this);
    QModelIndex index = ui->tableView->currentIndex();
    if (index.isValid()) {
        menu.addAction(ui->actionGoto);
        menu.addAction(ui->actionInsertCut);
//        menu.addAction(ui->actionOpen);
        menu.addAction(ui->actionRemove);
        menu.addSeparator();
    }
    menu.addAction(ui->actionRemoveAll);
    menu.addAction(ui->actionClean);
    menu.addAction(ui->actionWipe);
    menu.exec(mapToGlobal(pos));
}

void MeltedPlaylistDock::on_actionInsertCut_triggered()
{
    if (ui->tableView->currentIndex().isValid()) {
        emit insertRequested(ui->tableView->currentIndex().row());
    }
}

void MeltedPlaylistDock::on_actionOpen_triggered()
{
    // TODO: open this clip in the player for adjusting in/out without seeking melted unit.
}

void MeltedPlaylistDock::on_actionGoto_triggered()
{
    on_tableView_doubleClicked(ui->tableView->currentIndex());
}

void MeltedPlaylistDock::on_actionRemoveAll_triggered()
{
    QMessageBox dialog(QMessageBox::Warning,
                       qApp->applicationName(),
                        tr("\"Remove All\" will remove all of clips in the playlist.\n\n"
                           "IMPORTANT: You cannot Undo this action!\n\n"
                           "Do you want to continue?"),
                        QMessageBox::No | QMessageBox::Yes,
                        this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    int r = dialog.exec();
    if (r == QMessageBox::Yes)
        m_model.clear();
}

void MeltedPlaylistDock::on_actionWipe_triggered()
{
    QMessageBox dialog(QMessageBox::Warning,
                       qApp->applicationName(),
                        tr("\"Remove All\" will remove all of clips in the playlist.\n\n"
                           "IMPORTANT: You cannot Undo this action!\n\n"
                           "Do you want to continue?"),
                        QMessageBox::No | QMessageBox::Yes,
                        this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    int r = dialog.exec();
    if (r == QMessageBox::Yes)
        m_model.wipe();
}

void MeltedPlaylistDock::on_actionClean_triggered()
{
    QMessageBox dialog(QMessageBox::Warning,
                       qApp->applicationName(),
                       tr("\"Clean\" will remove all of clips in the playlist\n"
                          "except the currently playing clip.\n\n"
                          "IMPORTANT: You cannot Undo this action!\n\n"
                          "Do you want to continue?"),
                        QMessageBox::No | QMessageBox::Yes,
                        this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    int r = dialog.exec();
    if (r == QMessageBox::Yes)
        m_model.clean();
}

namespace MeltedPlaylist
{

TransportControl::TransportControl(MeltedPlaylistModel &model)
    : m_model(model)
{
}

void TransportControl::play(double speed)
{
    m_model.play(speed);
}

void TransportControl::pause()
{
    m_model.pause();
}

void TransportControl::stop()
{
    m_model.stop();
}

void TransportControl::seek(int position)
{
    m_model.seek(position);
}

void TransportControl::rewind()
{
    m_model.rewind();
}

void TransportControl::fastForward()
{
    m_model.fastForward();
}

void TransportControl::previous(int currentPosition)
{
    Q_UNUSED(currentPosition)
    m_model.previous();
}

void TransportControl::next(int currentPosition)
{
    Q_UNUSED(currentPosition)
    m_model.next();
}

void TransportControl::setIn(int in)
{
    if (in > 0)
        m_model.setIn(in);
}

void TransportControl::setOut(int out)
{
    if (out > 0)
        m_model.setOut(out);
}

AppendCommand::AppendCommand(MeltedPlaylistModel &model, const QString &clip, int in, int out, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_clip(clip)
    , m_in(in)
    , m_out(out)
    , m_skip(true)
{
    QFileInfo fi(m_clip);
    setText(QObject::tr("Append %1").arg(fi.fileName()));
}

void AppendCommand::redo()
{
    if (m_skip)
        m_skip = false;
    else
        m_model.append(m_clip, m_in, m_out, false);
}

void AppendCommand::undo()
{
    m_model.remove(m_model.rowCount() - 1, false);
}

RemoveCommand::RemoveCommand(MeltedPlaylistModel &model, QString &clip, int row, int in, int out, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_clip(clip)
    , m_row(row)
    , m_in(in)
    , m_out(out)
    , m_skip(true)
{
    QFileInfo fi(m_clip);
    setText(QObject::tr("Remove %1 at %2").arg(fi.fileName()).arg(row + 1));
}

void RemoveCommand::redo()
{
    if (m_skip)
        m_skip = false;
    else
        m_model.remove(m_row, false);
}

void RemoveCommand::undo()
{
    m_model.insert(m_clip, m_row, m_in, m_out, false);
}

InsertCommand::InsertCommand(MeltedPlaylistModel &model, const QString &clip, int row, int in, int out, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_clip(clip)
    , m_row(row)
    , m_in(in)
    , m_out(out)
    , m_skip(true)
{
    QFileInfo fi(m_clip);
    setText(QObject::tr("Insert %1 at %2").arg(fi.fileName()).arg(row + 1));
}

void InsertCommand::redo()
{
    if (m_skip)
        m_skip = false;
    else
        m_model.insert(m_clip, m_row, m_in, m_out, false);
}

void InsertCommand::undo()
{
    m_model.remove(m_row, false);
}

MoveCommand::MoveCommand(MeltedPlaylistModel &model, QString &clip, int from, int to, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_from(from)
    , m_to(to)
    , m_skip(true)
{
    QFileInfo fi(clip);
    setText(QObject::tr("Move %1 from %2 to %3").arg(fi.fileName()).arg(from + 1).arg(to + 1));
}

void MoveCommand::redo()
{
    if (m_skip)
        m_skip = false;
    else
        m_model.move(m_from, m_to, false);
}

void MoveCommand::undo()
{
    m_model.move(m_to, m_from, false);
}

}  // namespace MeltedPlaylist
