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

#ifndef FILTER_H
#define FILTER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <MltFilter.h>
#include "models/attachedfiltersmodel.h"

class QmlFilter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isNew READ isNew)
    Q_PROPERTY(QString mlt_service WRITE create)

public:
    explicit QmlFilter(AttachedFiltersModel& model, int row, QObject *parent = 0);
    ~QmlFilter();

    Q_INVOKABLE bool isNew() const;
    Q_INVOKABLE QString get(QString name);
    Q_INVOKABLE void set(QString name, QString value);
    Q_INVOKABLE void set(QString name, double value);
    Q_INVOKABLE void set(QString name, int value);

public slots:
    void create(const QString& name);

signals:
    void serviceNameChanged();

private:
    QString m_serviceName;
    AttachedFiltersModel& m_model;
    Mlt::Filter* m_filter;
};

#endif // FILTER_H
