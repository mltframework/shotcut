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

#include "mvcpthread.h"
#include <mvcp_remote.h>
#include <QVariant>

MvcpThread::MvcpThread(const QString &address, quint16 port, QObject *parent)
    : QThread(parent)
    , m_address(address)
    , m_port(port)
    , m_quit(false)
{
}

MvcpThread::~MvcpThread()
{
    m_mutex.lock();
    m_quit = true;
    m_cond.wakeOne();
    m_mutex.unlock();
    wait();
}

void MvcpThread::run()
{
    mvcp_parser parser = mvcp_parser_init_remote(
                const_cast<char*>(m_address.toUtf8().constData()), m_port);
    mvcp_response response = mvcp_parser_connect(parser);
    if (response) {
        mvcp lmvcp = mvcp_init(parser);
        mvcp_response_close(response);
        int command = COMMAND_INVALID;
        QObject* parameters = 0;

        m_mutex.lock();
        if (!m_commands.isEmpty()) {
            parameters = m_commands.takeFirst();
            command = parameters->property("command").toInt();
        }
        m_mutex.unlock();

        while (!m_quit) {
            if (parameters) {
                switch (command) {
                case COMMAND_ULS:
                    do_uls(lmvcp, parameters);
                    break;
                case COMMAND_CLS:
                    do_cls(lmvcp, parameters);
                    break;
                case COMMAND_USTA:
                    do_usta(lmvcp, parameters);
                    break;
                default:
                    break;
                }
            }
            m_mutex.lock();
            while (!m_quit && m_commands.isEmpty())
                m_cond.wait(&m_mutex);
            if (!m_commands.isEmpty()) {
                parameters = m_commands.takeFirst();
                command = parameters->property("command").toInt();
            }
            m_mutex.unlock();
        }
        mvcp_close(lmvcp);
    }
    mvcp_parser_close(parser);
}

void MvcpThread::uls()
{
    QMutexLocker locker(&m_mutex);
    QObject* command = new QObject;
    command->setProperty("command", COMMAND_ULS);
    m_commands.append(command);
    if (!isRunning())
        start();
    else
        m_cond.wakeOne();
}

void MvcpThread::do_uls(mvcp amvcp, QObject* parameters)
{
    delete parameters;
    mvcp_units units = mvcp_units_init(amvcp);
    QStringList unitList;
    for (int i = 0; i < mvcp_units_count(units); i++) {
        mvcp_unit_entry_t unit;
        mvcp_units_get(units, i, &unit);
        unitList << QString::fromUtf8(unit.guid);
    }
    mvcp_units_close(units);
    emit ulsResult(unitList);
}

void MvcpThread::cls(QString path, QObject* parent)
{
    QMutexLocker locker(&m_mutex);
    parent->setProperty("command", COMMAND_CLS);
    parent->setProperty("path", path);
    m_commands.append(parent);
    if (!isRunning())
        start();
    else
        m_cond.wakeOne();
}

void MvcpThread::do_cls(mvcp a_mvcp, QObject* parent)
{
    QObjectList* result = new QObjectList;
    mvcp_dir dir = mvcp_dir_init(a_mvcp, parent->property("path").toString().toUtf8().constData());
    int n = mvcp_dir_count(dir);
    for (int i = 0; i < n; i++) {
        mvcp_dir_entry_t entry;
        mvcp_dir_get(dir, i, &entry);
        QObject* o = new QObject;
        o->setObjectName(QString::fromUtf8(entry.full));
        o->setProperty("name", QString::fromUtf8(entry.name));
        o->setProperty("dir", entry.dir);
        o->setProperty("size", entry.size);
        result->append(o);
    }
    mvcp_dir_close(dir);
    emit clsResult(parent, result);
}

void MvcpThread::usta(quint8 unit)
{
    QMutexLocker locker(&m_mutex);
    QObject* command = new QObject;
    command->setProperty("command", COMMAND_USTA);
    command->setProperty("unit", unit);
    m_commands.append(command);
    if (!isRunning())
        start();
    else
        m_cond.wakeOne();
}

void MvcpThread::do_usta(mvcp a_mvcp, QObject* parameters)
{
    mvcp_status_t status;
    mvcp_unit_status(a_mvcp, parameters->property("unit").toInt(), &status);
    delete parameters;
    QString s;
    switch (status.status) {
    case unit_unknown:      s = tr("unknown"); break;
    case unit_undefined:    s = tr("undefined"); break;
    case unit_offline:      s = tr("offline"); break;
    case unit_not_loaded:   s = tr("unloaded"); break;
    case unit_stopped:      s = tr("stopped"); break;
    case unit_playing:      s = tr("playing"); break;
    case unit_paused:       s = tr("paused"); break;
    case unit_disconnected: s = tr("disconnected"); break;
    }
    QObject* result = new QObject;
    result->setProperty("unit", status.unit);
    result->setProperty("status", s);
    result->setProperty("clip", QString::fromUtf8(status.clip));
    result->setProperty("position", status.position);
    result->setProperty("generation", status.generation);
    result->setProperty("clip_index", status.clip_index);
    emit ustaResult(result);
}
