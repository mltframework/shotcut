/*
 * Copyright (c) 2016-2026 Meltytech, LLC
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

#include "shotcut_mlt_properties.h"

#include <MltProducer.h>
#include <QObject>
#include <QRectF>
#include <QString>
#include <QVariant>

class QmlProducer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int in READ in() NOTIFY inChanged)
    Q_PROPERTY(int out READ out() NOTIFY outChanged)
    Q_PROPERTY(int aspectRatio READ aspectRatio() NOTIFY producerChanged)
    Q_PROPERTY(int duration READ duration() NOTIFY durationChanged)
    Q_PROPERTY(int length READ length() NOTIFY lengthChanged)
    Q_PROPERTY(QString resource READ resource() NOTIFY producerChanged)
    Q_PROPERTY(QString mlt_service READ mlt_service() NOTIFY producerChanged)
    Q_PROPERTY(QString hash READ hash() NOTIFY producerChanged)
    Q_PROPERTY(QString name READ name() NOTIFY producerChanged)
    Q_PROPERTY(int fadeIn READ fadeIn NOTIFY producerChanged)
    Q_PROPERTY(int fadeOut READ fadeOut NOTIFY producerChanged)
    Q_PROPERTY(double speed READ speed NOTIFY producerChanged)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(double displayAspectRatio READ displayAspectRatio NOTIFY producerChanged)

public:
    explicit QmlProducer(QObject *parent = 0);

    int in();
    int out();
    double aspectRatio();
    int duration() { return m_producer.is_valid() ? out() - in() + 1 : 0; }
    int length() { return m_producer.is_valid() ? m_producer.get_length() : 0; }
    QString resource();
    QString mlt_service()
    {
        return m_producer.is_valid() ? m_producer.get("mlt_service") : QString();
    }
    QString hash()
    {
        return m_producer.is_valid() ? m_producer.get(kShotcutHashProperty) : QString();
    }
    QString name();
    const QVariantList *audioLevels();
    int fadeIn();
    int fadeOut();
    double speed();
    int position() const { return m_position; }
    void setPosition(int position);
    void seek(int position);
    Mlt::Producer &producer() { return m_producer; }
    Q_INVOKABLE Mlt::Producer *getMltProducer() { return &m_producer; }
    Q_INVOKABLE void audioLevelsReady(const QPersistentModelIndex &index);
    Q_INVOKABLE void remakeAudioLevels();
    double displayAspectRatio();
    Q_INVOKABLE QString get(QString name, int position = -1);
    Q_INVOKABLE double getDouble(QString name, int position = -1);
    Q_INVOKABLE QRectF getRect(QString name, int position = -1);
    Q_INVOKABLE bool outOfBounds();
    Q_INVOKABLE void newGlaxnimateFile(const QString &filename);
    Q_INVOKABLE void launchGlaxnimate(const QString &filename = QString()) const;

signals:
    void producerChanged();
    void positionChanged(int position);
    void seeked(int position);
    void inChanged(int delta);
    void outChanged(int delta);
    void audioLevelsChanged();
    void durationChanged();
    void lengthChanged();

public slots:
    void setProducer(Mlt::Producer &producer);
    void remakeAudioLevels(bool isKeyframesVisible);

private:
    Mlt::Producer m_producer;
    int m_position;
};

#endif // QMLPRODUCER_H
