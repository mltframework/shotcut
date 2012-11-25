/*
 * Copyright (c) 2012 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * GL shader based on BSD licensed code from Peter Bengtsson:
 * http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
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

MeltedServerDock::MeltedServerDock(QWidget *parent)
    : QDockWidget(parent)
    , ui(new Ui::MeltedServerDock)
    , m_parser(0)
    , m_mvcp(0)
{
    ui->setupUi(this);
    m_console = new QConsole(this);
    m_console->setDisabled(true);
    m_console->hide();
    ui->splitter->addWidget(m_console);
    connect(m_console, SIGNAL(commandExecuted(QString)), this, SLOT(onCommandExecuted(QString)));
    ui->treeView->header()->setResizeMode(QHeaderView::ResizeToContents);
    ui->treeView->setDisabled(true);
    MeltedUnitsModel* unitsModel = new MeltedUnitsModel(this);
    ui->unitsTableView->setModel(unitsModel);
    connect(this, SIGNAL(connected(MvcpThread*)), unitsModel, SLOT(onConnected(MvcpThread*)));
    connect(this, SIGNAL(disconnected()), unitsModel, SLOT(onDisconnected()));
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

void MeltedServerDock::on_lineEdit_returnPressed()
{
    ui->connectButton->setChecked(true);
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
        delete ui->treeView->model();
        ui->treeView->setDisabled(true);
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
            ui->connectButton->setText(tr("Disconnect"));
            emit connected(m_mvcp);
            emit connected(address[0], port);
            ui->treeView->setModel(new MeltedClipsModel(m_mvcp));
            ui->treeView->setEnabled(true);
        }
        else {
            ui->connectButton->setChecked(false);
        }
    } else {
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

