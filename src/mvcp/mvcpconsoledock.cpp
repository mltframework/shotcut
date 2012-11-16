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

#include "mvcpconsoledock.h"
#include "ui_mvcpconsoledock.h"
#include "qconsole.h"

MvcpConsoleDock::MvcpConsoleDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::MvcpConsoleDock)
{
    ui->setupUi(this);
    m_console = new QConsole(this);
    widget()->layout()->addWidget(m_console);
    connect(m_console, SIGNAL(commandExecuted(QString)), this, SLOT(onCommandExecuted(QString)));
}

MvcpConsoleDock::~MvcpConsoleDock()
{
    if (m_parser)
        mvcp_parser_close(m_parser);
    delete ui;
}

void MvcpConsoleDock::on_lineEdit_returnPressed()
{
    QStringList address = ui->lineEdit->text().split(':');
    int port = address.size() > 1 ? QString(address[1]).toInt() : 5250;
    m_parser = mvcp_parser_init_remote(
                const_cast<char*>(address[0].toUtf8().constData()), port);
    mvcp_response response = mvcp_parser_connect( m_parser );
    if (response) {
        for (int index = 0; index < mvcp_response_count(response); index++)
            m_console->append(QString::fromUtf8(mvcp_response_get_line(response, index)).append('\n'));
        mvcp_response_close(response);
        m_console->setPrompt("> ");
        m_console->setFocus();
    }
}

void MvcpConsoleDock::onCommandExecuted(QString command)
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
    if (command.toLower() == "bye") {
        mvcp_parser_close(m_parser);
        m_parser = NULL;
        m_console->setPrompt("");
        m_console->reset();
        ui->lineEdit->setFocus();
    }
}
