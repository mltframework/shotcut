/*
 * Copyright (c) 2013 Meltytech, LLC
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

#include "qmlfilter.h"
#include "mltcontroller.h"

QmlFilter::QmlFilter(AttachedFiltersModel& model, int row, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_filter(row >= 0? model.filterForRow(row) : 0)
{
}

QmlFilter::~QmlFilter()
{
    delete m_filter;
}

bool QmlFilter::isNew() const
{
    return !m_filter;
}

QString QmlFilter::get(QString name)
{
    if (m_filter)
        return QString::fromUtf8(m_filter->get(name.toUtf8().constData()));
    else
        return QString();
}

void QmlFilter::set(QString name, QString value)
{
    if (!m_filter) return;
    m_filter->set(name.toUtf8().constData(), value.toUtf8().constData());
    MLT.refreshConsumer();
}

void QmlFilter::set(QString name, double value)
{
    if (!m_filter) return;
    m_filter->set(name.toUtf8().constData(), value);
    MLT.refreshConsumer();
}

void QmlFilter::set(QString name, int value)
{
    if (!m_filter) return;
    m_filter->set(name.toUtf8().constData(), value);
    MLT.refreshConsumer();
}

void QmlFilter::create(const QString &mlt_service)
{
    if (!m_filter)
        m_filter = m_model.add(mlt_service);
}

void QmlFilter::setName(const QString &name)
{
    if (m_filter)
        m_filter->set("shotcut:name", name.toUtf8().constData());
}

void QmlFilter::setPath(const QString &path)
{
    m_path = path;
}
