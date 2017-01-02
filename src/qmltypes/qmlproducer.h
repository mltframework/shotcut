/*
 * Copyright (c) 2016-2017 Meltytech, LLC
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

#ifndef QMLPRODUCER_H
#define QMLPRODUCER_H

#include <QObject>
#include <QString>
#include <QVariant>

#include <MltProducer.h>
#include "shotcut_mlt_properties.h"

class QmlProducer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int in READ in())
    Q_PROPERTY(int out READ out())
    Q_PROPERTY(int aspectRatio READ aspectRatio())
    Q_PROPERTY(int duration READ duration())
    Q_PROPERTY(QString resource READ resource())
    Q_PROPERTY(QString mlt_service READ mlt_service())
    Q_PROPERTY(QString hash READ hash())
    Q_PROPERTY(QString name READ name())
    Q_PROPERTY(QVariant audioLevels READ audioLevels)
    Q_PROPERTY(int fadeIn READ fadeIn)
    Q_PROPERTY(int fadeOut READ fadeOut)
    Q_PROPERTY(double speed READ speed)

public:
    explicit QmlProducer(Mlt::Producer& producer, QObject *parent = 0);

    int in();
    int out();
    double aspectRatio();
    int duration() {return out() - in() + 1;}
    QString resource();
    QString mlt_service() {return m_producer.get("mlt_service");}
    QString hash() {return m_producer.get(kShotcutHashProperty);}
    QString name();
    QVariant audioLevels();
    int fadeIn();
    int fadeOut();
    double speed();

signals:

public slots:

private:
    Mlt::Producer m_producer;
};

#endif // QMLPRODUCER_H
