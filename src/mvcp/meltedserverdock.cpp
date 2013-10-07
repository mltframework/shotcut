/*
 * Copyright (c) 2012-2013 Meltytech, LLC
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

#include "meltedserverdock.h"
#include "ui_meltedserverdock.h"
#include "qconsole.h"
#include "meltedclipsmodel.h"
#include "meltedunitsmodel.h"
#include "mltcontroller.h"
#include "settings.h"
#include <QCompleter>
#include <QStringListModel>
#include <QFileDialog>

MeltedServerDock::MeltedServerDock(QWidget *parent)
    : QDockWidget(parent)
    , ui(new Ui::MeltedServerDock)
    , m_parser(0)
    , m_mvcp(0)
{
    ui->setupUi(this);

    // setup console
    m_console = new QConsole(this);
    m_console->setDisabled(true);
    m_console->hide();
    ui->splitter->addWidget(m_console);
    connect(m_console, SIGNAL(commandExecuted(QString)), this, SLOT(onCommandExecuted(QString)));

    // setup clips tree
    ui->treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->treeView->setDisabled(true);
    ui->treeView->setDragEnabled(true);

    // setup units table
    MeltedUnitsModel* unitsModel = new MeltedUnitsModel(this);
    ui->unitsTableView->setModel(unitsModel);
    connect(this, SIGNAL(connected(MvcpThread*)), unitsModel, SLOT(onConnected(MvcpThread*)));
    connect(this, SIGNAL(disconnected()), unitsModel, SLOT(onDisconnected()));
    connect(unitsModel, SIGNAL(positionUpdated(quint8,int,double,int,int,int,bool)), this, SLOT(onPositionUpdated(quint8,int,double,int,int,int,bool)));

    // setup server field
    QStringList servers = Settings.meltedServers();
    QCompleter* completer = new QCompleter(servers, this);
    ui->lineEdit->setCompleter(completer);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    if (!servers.isEmpty())
        ui->lineEdit->setText(servers[0]);
}

MeltedServerDock::~MeltedServerDock()
{
    if (m_parser)
        mvcp_parser_close(m_parser);
    delete ui;
}

QAbstractItemModel *MeltedServerDock::unitsModel() const
{
    return ui->unitsTableView->model();
}

QAbstractItemModel *MeltedServerDock::clipsModel() const
{
    return ui->treeView->model();
}

void MeltedServerDock::on_lineEdit_returnPressed()
{
    QString s = ui->lineEdit->text();
    if (!s.isEmpty()) {
        ui->connectButton->setChecked(true);
        QStringList servers = Settings.meltedServers();
        servers.removeOne(s);
        servers.prepend(s);
        while (servers.count() > 20)
            servers.removeLast();
        Settings.setMeltedServers(servers);
        QCompleter* completer = ui->lineEdit->completer();
        completer->setModel(new QStringListModel(servers, completer));
    }
}

void MeltedServerDock::onCommandExecuted(QString command)
{
    if (!m_parser || command.isEmpty())
        return;

    mvcp_response response = mvcp_parser_execute(m_parser,
        const_cast<char*>(command.toUtf8().constData()));
    if (response) {
        for (int index = 0; index < mvcp_response_count(response); index++) {
            QString line = QString::fromUtf8(mvcp_response_get_line(response, index));
            m_console->append(line);
        }
        mvcp_response_close(response);
    }
    QString s = command.toLower();
    if (s == "bye" || s == "exit")
        ui->connectButton->setChecked(false);
}

void MeltedServerDock::on_connectButton_toggled(bool checked)
{
    if (m_parser) {
        delete m_mvcp;
        m_mvcp = 0;
        mvcp_parser_close(m_parser);
        m_parser = 0;
        m_console->setPrompt("");
        m_console->reset();
        m_console->setDisabled(true);
        ui->lineEdit->setFocus();
        ui->treeView->setDisabled(true);
        ui->menuButton->setDisabled(true);
        m_mappedClipsRoot = "";
    }
    if (checked) {
        QStringList address = ui->lineEdit->text().split(':');
        quint16 port = address.size() > 1 ? QString(address[1]).toUInt() : 5250;
        m_parser = mvcp_parser_init_remote(
                    const_cast<char*>(address[0].toUtf8().constData()), port);
        mvcp_response response = mvcp_parser_connect( m_parser );
        if (response) {
            for (int index = 0; index < mvcp_response_count(response); index++)
                m_console->append(QString::fromUtf8(mvcp_response_get_line(response, index)).append('\n'));
            mvcp_response_close(response);
            m_console->setEnabled(true);
            m_console->setPrompt("> ");
            m_console->setFocus();
            m_mvcp = new MvcpThread(address[0], port);
            ui->treeView->setModel(new MeltedClipsModel(m_mvcp));
            ui->treeView->setEnabled(true);
            emit connected(m_mvcp);
            emit connected(address[0], port);
            ui->stackedWidget->setCurrentIndex(1);
            ui->connectButton->setText(tr("Disconnect"));
            ui->menuButton->setEnabled(true);
        }
        else {
            ui->connectButton->setChecked(false);
        }
    } else {
        ui->stackedWidget->setCurrentIndex(0);
        ui->connectButton->setText(tr("Connect"));
        emit disconnected();
    }
}

void MeltedServerDock::on_unitsTableView_clicked(const QModelIndex &index)
{
    emit unitActivated(index.row());
}

void MeltedServerDock::on_unitsTableView_doubleClicked(const QModelIndex &index)
{
    emit unitOpened(index.row());
}

void MeltedServerDock::on_consoleButton_clicked(bool checked)
{
    m_console->setVisible(checked);
    if (checked)
        m_console->setFocus();
}

void MeltedServerDock::onPositionUpdated(quint8 unit, int position, double fps, int in, int out, int length, bool isPlaying)
{
    if (unit == ui->unitsTableView->currentIndex().row())
        emit positionUpdated(position, fps, in, out, length, isPlaying);
}

void MeltedServerDock::onAppendRequested()
{
    QString resource;
    if (MLT.producer() && MLT.producer()->get("resource"))
        resource = QString::fromUtf8(MLT.producer()->get("resource"));
    if (!m_mappedClipsRoot.isEmpty() && resource.startsWith(m_mappedClipsRoot)) {
        resource.remove(0, m_mappedClipsRoot.length());
        emit append(resource, MLT.producer()->get_in(), MLT.producer()->get_out());
    }
    else if (ui->treeView->currentIndex().isValid()) {
        emit append(clipsModel()->data(ui->treeView->currentIndex(), Qt::UserRole).toString());
    }
}

void MeltedServerDock::onInsertRequested(int row)
{
    QString resource;
    if (MLT.producer() && MLT.producer()->get("resource"))
        resource = QString::fromUtf8(MLT.producer()->get("resource"));
    if (!m_mappedClipsRoot.isEmpty() && resource.startsWith(m_mappedClipsRoot)) {
        resource.remove(0, m_mappedClipsRoot.length());
        emit insert(resource, row, MLT.producer()->get_in(), MLT.producer()->get_out());
    }
    else if (ui->treeView->currentIndex().isValid()) {
        emit insert(clipsModel()->data(ui->treeView->currentIndex(), Qt::UserRole).toString(), row);
    }
}

void MeltedServerDock::on_unitsTableView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->unitsTableView->currentIndex();
    if (index.isValid()) {
        QMenu menu(this);
        menu.addAction(ui->actionFast_Forward);
        menu.addAction(ui->actionPause);
        menu.addAction(ui->actionPlay);
        menu.addAction(ui->actionRewind);
        menu.addAction(ui->actionStop);
        menu.exec(mapToGlobal(pos));
    }
}

QAction* MeltedServerDock::actionFastForward() const
{
    return ui->actionFast_Forward;
}

QAction * MeltedServerDock::actionPause() const
{
    return ui->actionPause;
}

QAction * MeltedServerDock::actionPlay() const
{
    return ui->actionPlay;
}

QAction * MeltedServerDock::actionRewind() const
{
    return ui->actionRewind;
}

QAction * MeltedServerDock::actionStop() const
{
    return ui->actionStop;
}

void MeltedServerDock::on_actionMapClipsRoot_triggered()
{
    m_mappedClipsRoot = QFileDialog::getExistingDirectory(this, tr("Choose Directory"));
}

void MeltedServerDock::on_menuButton_clicked()
{
    QPoint pos = ui->menuButton->mapToParent(QPoint(0, 0));
    QMenu menu(this);
    QModelIndex index = ui->unitsTableView->currentIndex();
    if (index.isValid()) {
        menu.addAction(ui->actionFast_Forward);
        menu.addAction(ui->actionPause);
        menu.addAction(ui->actionPlay);
        menu.addAction(ui->actionRewind);
        menu.addAction(ui->actionStop);
        menu.addSeparator();
    }
    menu.addAction(ui->actionMapClipsRoot);
    menu.exec(mapToGlobal(pos));
}

void MeltedServerDock::on_treeView_doubleClicked(const QModelIndex &index)
{
    if (!m_mappedClipsRoot.isEmpty()) {
        QString resource = clipsModel()->data(index, Qt::UserRole).toString();
        resource.prepend(m_mappedClipsRoot);
        emit openLocal(resource);
    }
}
