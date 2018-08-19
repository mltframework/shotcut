/*
 * Copyright (c) 2013-2018 Meltytech, LLC
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
#include <QRectF>
#include <MltFilter.h>
#include <MltProducer.h>
#include <MltAnimation.h>

#include "qmlmetadata.h"
#include "shotcut_mlt_properties.h"

class AbstractJob;

class QmlFilter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isNew READ isNew)
    Q_PROPERTY(QString path READ path)
    Q_PROPERTY(QStringList presets READ presets NOTIFY presetsChanged)
    Q_PROPERTY(int in READ in WRITE setIn NOTIFY inChanged)
    Q_PROPERTY(int out READ out WRITE setOut NOTIFY outChanged)
    Q_PROPERTY(int animateIn READ animateIn WRITE setAnimateIn NOTIFY animateInChanged)
    Q_PROPERTY(int animateOut READ animateOut WRITE setAnimateOut NOTIFY animateOutChanged)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool blockSignals WRITE blockSignals)

public:
    enum TimeFormat
    {
        TIME_FRAMES,
        TIME_CLOCK,
        TIME_TIMECODE_DF,
        TIME_TIMECODE_NDF,
    };
    Q_ENUMS(TimeFormat)

    explicit QmlFilter();
    explicit QmlFilter(Mlt::Filter& mltFilter, const QmlMetadata* metadata, QObject *parent = 0);
    ~QmlFilter();

    bool isNew() const { return m_isNew; }
    void setIsNew(bool isNew) { m_isNew = isNew; }

    Q_INVOKABLE QString get(QString name, int position = -1);
    Q_INVOKABLE double getDouble(QString name, int position = -1);
    Q_INVOKABLE QRectF getRect(QString name, int position = -1);
    Q_INVOKABLE void set(QString name, QString value, int position = -1);
    Q_INVOKABLE void set(QString name, double value,
                         int position = -1, mlt_keyframe_type keyframeType = mlt_keyframe_type(-1));
    Q_INVOKABLE void set(QString name, int value,
                         int position = -1, mlt_keyframe_type keyframeType = mlt_keyframe_type(-1));
    Q_INVOKABLE void set(QString name, bool value,
                         int position = -1, mlt_keyframe_type keyframeType = mlt_keyframe_type(-1));
    Q_INVOKABLE void set(QString name, double x, double y, double width, double height, double opacity = 1.0,
                         int position = -1, mlt_keyframe_type keyframeType = mlt_keyframe_type(-1));
    Q_INVOKABLE void set(QString name, const QRectF& rect, double opacity = 1.0,
                         int position = -1, mlt_keyframe_type keyframeType = mlt_keyframe_type(-1));
    QString path() const { return m_path; }
    Q_INVOKABLE void loadPresets();
    QStringList presets() const { return m_presets; }
    /// returns the index of the new preset
    Q_INVOKABLE int  savePreset(const QStringList& propertyNames, const QString& name = QString());
    Q_INVOKABLE void deletePreset(const QString& name);
    Q_INVOKABLE void analyze(bool isAudio = false);
    Q_INVOKABLE static int framesFromTime(const QString& time);
    Q_INVOKABLE static QString timeFromFrames(int frames, TimeFormat format = TIME_TIMECODE_DF);
    Q_INVOKABLE void getHash();
    Mlt::Producer& producer() { return m_producer; }
    int in();
    void setIn(int value);
    int out();
    void setOut(int value);
    Mlt::Filter& filter() { return m_filter; }
    int animateIn();
    void setAnimateIn(int value);
    int animateOut();
    void setAnimateOut(int value);
    int duration();
    Q_INVOKABLE void resetProperty(const QString& name);
    Q_INVOKABLE void clearSimpleAnimation(const QString& name);
    Mlt::Animation getAnimation(const QString& name);
    Q_INVOKABLE int keyframeCount(const QString& name);

public slots:
    void preset(const QString& name);

signals:
    void presetsChanged();
    void analyzeFinished(bool isSuccess);
    void changed(); /// Use to let UI and VUI QML signal updates to each other.
    void changed(QString name);
    void inChanged(int delta);
    void outChanged(int delta);
    void animateInChanged();
    void animateOutChanged();
    void durationChanged();

private:
    const QmlMetadata* m_metadata;
    Mlt::Filter m_filter;
    Mlt::Producer m_producer;
    QString m_path;
    bool m_isNew;
    QStringList m_presets;
    
    QString objectNameOrService();
    int keyframeIndex(Mlt::Animation& animation, int position);
    mlt_keyframe_type getKeyframeType(Mlt::Animation& animation, int position, mlt_keyframe_type defaultType = mlt_keyframe_linear);
};

class AnalyzeDelegate : public QObject
{
    Q_OBJECT
public:
    explicit AnalyzeDelegate(Mlt::Filter& filter);

public slots:
    void onAnalyzeFinished(AbstractJob *job, bool isSuccess);

private:
    Mlt::Filter m_filter;
};

#endif // FILTER_H
