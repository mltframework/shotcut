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

#include "qmlfilter.h"
#include "mltcontroller.h"
#include "mainwindow.h"
#include "controllers/filtercontroller.h"
#include "jobqueue.h"
#include "jobs/meltjob.h"
#include "shotcut_mlt_properties.h"
#include "settings.h"
#include <Logger.h>

#include <QDir>
#include <QIODevice>
#include <QTemporaryFile>
#include <QFile>
#include <QtXml>
#include <MltProducer.h>

QmlFilter::QmlFilter()
    : QObject(0)
    , m_metadata(0)
    , m_filter(mlt_filter(0))
    , m_producer(mlt_producer(0))
    , m_isNew(false)
{
    connect(this, SIGNAL(inChanged(int)), this, SIGNAL(durationChanged()));
    connect(this, SIGNAL(outChanged(int)), this, SIGNAL(durationChanged()));
}

QmlFilter::QmlFilter(Mlt::Filter& mltFilter, const QmlMetadata* metadata, QObject* parent)
    : QObject(parent)
    , m_metadata(metadata)
    , m_filter(mltFilter)
    // Every attached filter has a service property that points to the service to
    // which it is attached.
    , m_producer(mlt_producer(m_filter.is_valid()? m_filter.get_data("service") : 0))
    , m_path(m_metadata->path().absolutePath().append('/'))
    , m_isNew(false)
{
    connect(this, SIGNAL(changed(QString)), SIGNAL(changed()));
}

QmlFilter::~QmlFilter()
{
}

QString QmlFilter::get(QString name, int position)
{
    if (m_filter.is_valid()) {
        const char* propertyName = name.toUtf8().constData();
        if (position < 0)
            return QString::fromUtf8(m_filter.get(propertyName));
        else
            return QString::fromUtf8(m_filter.anim_get(propertyName, position, duration()));
    } else {
        return QString();
    }
}

double QmlFilter::getDouble(QString name, int position)
{
    if (m_filter.is_valid()) {
        const char* propertyName = name.toUtf8().constData();
        if (position < 0)
            return m_filter.get_double(propertyName);
        else
            return m_filter.anim_get_double(propertyName, position, duration());
    } else {
        return 0.0;
    }
}

QRectF QmlFilter::getRect(QString name, int position)
{
    if (!m_filter.is_valid()) return QRectF();
    const char* s = m_filter.get(name.toUtf8().constData());
    if (s) {
        const char* propertyName = name.toUtf8().constData();
        mlt_rect rect;
        if (position < 0) {
            rect = m_filter.get_rect(propertyName);
        } else {
            rect = m_filter.anim_get_rect(propertyName, position, duration());
        }
        if (::strchr(s, '%')) {
            return QRectF(qRound(rect.x * MLT.profile().width()),
                          qRound(rect.y * MLT.profile().height()),
                          qRound(rect.w * MLT.profile().width()),
                          qRound(rect.h * MLT.profile().height()));
        } else {
            return QRectF(rect.x, rect.y, rect.w, rect.h);
        }
    } else {
        return QRectF(0.0, 0.0, 0.0, 0.0);
    }
}

void QmlFilter::set(QString name, QString value, int position)
{
    if (!m_filter.is_valid()) return;
    if (position < 0) {
        if (qstrcmp(m_filter.get(name.toUtf8().constData()), value.toUtf8().constData())) {
            m_filter.set(name.toUtf8().constData(), value.toUtf8().constData());
            emit changed(name);
        }
    } else {
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_filter.get_animation(name.toUtf8().constData()));
        if (!animation.is_valid() || !animation.is_key(position)
                || value != m_filter.anim_get(name.toUtf8().constData(), position, duration())) {
            m_filter.anim_set(name.toUtf8().constData(), value.toUtf8().constData(), position, duration());
            emit changed(name);
        }
    }
}

void QmlFilter::set(QString name, double value, int position, mlt_keyframe_type keyframeType)
{
    if (!m_filter.is_valid()) return;
    if (position < 0) {
        if (!m_filter.get(name.toUtf8().constData())
            || m_filter.get_double(name.toUtf8().constData()) != value) {
            double delta = value - m_filter.get_double(name.toUtf8().constData());
            m_filter.set(name.toUtf8().constData(), value);
            emit changed(name);
            if (name == "in") {
                emit inChanged(delta);
            } else if (name == "out") {
                emit outChanged(delta);
            }
        }
    } else {
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_filter.get_animation(name.toUtf8().constData()));
        if (!animation.is_valid() || !animation.is_key(position)
                || value != m_filter.anim_get_double(name.toUtf8().constData(), position, duration())) {
            mlt_keyframe_type type = getKeyframeType(animation, position, keyframeType);
            m_filter.anim_set(name.toUtf8().constData(), value, position, duration(), type);
            emit changed(name);
        }
    }
}

void QmlFilter::set(QString name, int value, int position, mlt_keyframe_type keyframeType)
{
    if (!m_filter.is_valid()) return;
    if (position < 0) {
        if (!m_filter.get(name.toUtf8().constData())
            || m_filter.get_int(name.toUtf8().constData()) != value) {
            int delta = value - m_filter.get_int(name.toUtf8().constData());
            m_filter.set(name.toUtf8().constData(), value);
            emit changed(name);
            if (name == "in") {
                emit inChanged(delta);
            } else if (name == "out") {
                emit outChanged(delta);
            }
        }
    } else {
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_filter.get_animation(name.toUtf8().constData()));
        if (!animation.is_valid() || !animation.is_key(position)
                || value != m_filter.anim_get_int(name.toUtf8().constData(), position, duration())) {
            mlt_keyframe_type type = getKeyframeType(animation, position, keyframeType);
            m_filter.anim_set(name.toUtf8().constData(), value, position, duration(), type);
            emit changed(name);
        }
    }
}

void QmlFilter::set(QString name, bool value, int position, mlt_keyframe_type keyframeType)
{
    set(name, value? 1 : 0, position, keyframeType);
}

void QmlFilter::set(QString name, double x, double y, double width, double height, double opacity,
                    int position, mlt_keyframe_type keyframeType)
{
    if (!m_filter.is_valid()) return;
    if (position < 0) {
        mlt_rect rect = m_filter.get_rect(name.toUtf8().constData());
        if (!m_filter.get(name.toUtf8().constData()) || x != rect.x || y != rect.y
            || width != rect.w || height != rect.h || opacity != rect.o) {
            m_filter.set(name.toUtf8().constData(), x, y, width, height, opacity);
            emit changed(name);
        }
    } else {
        mlt_rect rect = m_filter.anim_get_rect(name.toUtf8().constData(), position, duration());
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_filter.get_animation(name.toUtf8().constData()));
        if (!animation.is_valid() || !animation.is_key(position)
                || x != rect.x || y != rect.y || width != rect.w || height != rect.h || opacity != rect.o) {
            rect.x = x;
            rect.y = y;
            rect.w = width;
            rect.h = height;
            rect.o = opacity;
            mlt_keyframe_type type = getKeyframeType(animation, position, keyframeType);
            m_filter.anim_set(name.toUtf8().constData(), rect, position, duration(), type);
            emit changed(name);
        }
    }
}

void QmlFilter::set(QString name, const QRectF& rect, double opacity, int position, mlt_keyframe_type keyframeType)
{
    set(name, rect.x(), rect.y(), rect.width(), rect.height(), opacity, position, keyframeType);
}

void QmlFilter::loadPresets()
{
    m_presets.clear();
    QDir dir(Settings.appDataLocation());
    if (dir.cd("presets")) {
        QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Executable);
        foreach (QString s, entries) {
            if (s == objectNameOrService() && dir.cd(s)) {
                m_presets.append("");
                m_presets.append(dir.entryList(QDir::Files | QDir::Readable));
                break;
            }
        }
    }
    emit presetsChanged();
}

int QmlFilter::savePreset(const QStringList &propertyNames, const QString &name)
{
    Mlt::Properties properties;
    QDir dir(Settings.appDataLocation());

    properties.pass_list(m_filter, propertyNames.join('\t').toLatin1().constData());

    if (!dir.exists())
        dir.mkpath(dir.path());
    if (!dir.cd("presets")) {
        if (dir.mkdir("presets"))
            dir.cd("presets");
    }
    if (!dir.cd(objectNameOrService())) {
        if (dir.mkdir(objectNameOrService()))
            dir.cd(objectNameOrService());
    }
    const QString preset = name.isEmpty()? tr("(defaults)") : name;
#if LIBMLT_VERSION_INT >= ((6<<16)+(9<<8))
    // Convert properties to YAML string.
    char* yamlStr = properties.serialise_yaml();
    QString yaml = yamlStr;
    free(yamlStr);
    // Save YAML to file
    QFile yamlFile(dir.filePath(preset));
    if(!yamlFile.open(QIODevice::WriteOnly)) {
        LOG_ERROR() << "Failed to save preset: " << dir.filePath(preset);
    }
    yamlFile.write(yaml.toUtf8());
    yamlFile.close();
#else
    properties.save(dir.filePath(preset).toUtf8().constData());
#endif
    loadPresets();
    return m_presets.indexOf(name);
}

void QmlFilter::deletePreset(const QString &name)
{
    QDir dir(Settings.appDataLocation());
    if (dir.cd("presets") && dir.cd(objectNameOrService()))
        QFile(dir.filePath(name)).remove();
    m_presets.removeOne(name);
    emit presetsChanged();
}

void QmlFilter::analyze(bool isAudio)
{
    Mlt::Service service(mlt_service(m_filter.get_data("service")));

    // get temp filename for input xml
    QTemporaryFile tmp;
    tmp.open();
    tmp.close();
    m_filter.set("results", NULL, 0);
    int disable = m_filter.get_int("disable");
    m_filter.set("disable", 0);
    MLT.saveXML(tmp.fileName(), &service);
    m_filter.set("disable", disable);

    // get temp filename for output xml
    QTemporaryFile tmpTarget;
    tmpTarget.open();
    tmpTarget.close();

    // parse xml
    QFile f1(tmp.fileName());
    f1.open(QIODevice::ReadOnly);
    QDomDocument dom(tmp.fileName());
    dom.setContent(&f1);
    f1.close();

    // add consumer element
    QDomElement consumerNode = dom.createElement("consumer");
    QDomNodeList profiles = dom.elementsByTagName("profile");
    if (profiles.isEmpty())
        dom.documentElement().insertAfter(consumerNode, dom.documentElement());
    else
        dom.documentElement().insertAfter(consumerNode, profiles.at(profiles.length() - 1));
    consumerNode.setAttribute("mlt_service", "xml");
    consumerNode.setAttribute("all", 1);
    if (isAudio)
        consumerNode.setAttribute("video_off", 1);
    else
        consumerNode.setAttribute("audio_off", 1);
    consumerNode.setAttribute("no_meta", 1);
    consumerNode.setAttribute("resource", tmpTarget.fileName());

    AbstractJob* job = new MeltJob(tmpTarget.fileName(), dom.toString(2),
        MLT.profile().frame_rate_num(), MLT.profile().frame_rate_den());
    if (job) {
        AnalyzeDelegate* delegate = new AnalyzeDelegate(m_filter);
        connect(job, &AbstractJob::finished, delegate, &AnalyzeDelegate::onAnalyzeFinished);
        connect(job, &AbstractJob::finished, this, &QmlFilter::analyzeFinished);
        QFileInfo info(QString::fromUtf8(service.get("resource")));
        job->setLabel(tr("Analyze %1").arg(info.fileName()));
        JOBS.add(job);
    }
}

int QmlFilter::framesFromTime(const QString &time)
{
    if (MLT.producer()) {
        return MLT.producer()->time_to_frames(time.toLatin1().constData());
    }
    return 0;
}

QString QmlFilter::timeFromFrames(int frames, TimeFormat format)
{
    if (MLT.producer()) {
        mlt_time_format mltFormat = mlt_time_smpte_df;
        switch ( format )
        {
            case TIME_FRAMES:
                mltFormat = mlt_time_frames;
                break;
            case TIME_CLOCK:
                mltFormat = mlt_time_clock;
                break;
            case TIME_TIMECODE_DF:
                mltFormat = mlt_time_smpte_df;
                break;
            case TIME_TIMECODE_NDF:
                mltFormat = mlt_time_smpte_ndf;
                break;
        }
        return MLT.producer()->frames_to_time(frames, mltFormat);
    }
    return QString();
}

void QmlFilter::getHash()
{
    if (m_filter.is_valid())
        MAIN.getHash(m_filter);
}

int QmlFilter::in()
{
    int result = 0;
    if (m_filter.is_valid()) {
        if (m_filter.get_int("in") == 0 && m_filter.get_int("out") == 0) { // undefined/always-on
            if (!m_producer.is_valid()) {
                result = 0;
            } else if (m_producer.get(kFilterInProperty)) {
                // Shots on the timeline will set the producer to the cut parent.
                // However, we want time-based filters such as fade in/out to use
                // the cut's in/out and not the parent's.
                result = m_producer.get_int(kFilterInProperty);
            } else {
                result = m_producer.get_in();
            }
        } else {
            result = m_filter.get_int("in");
        }
    }
    return result;
}

void QmlFilter::setIn(int value)
{
    set("in", value);
}

int QmlFilter::out()
{
    int result = 0;
    if (m_filter.is_valid()) {
        if (m_filter.get_int("in") == 0 && m_filter.get_int("out") == 0) { // undefined/always-on
            if (!m_producer.is_valid()) {
                result = 0;
            } else if (m_producer.get(kFilterOutProperty)) {
                // Shots on the timeline will set the producer to the cut parent.
                // However, we want time-based filters such as fade in/out to use
                // the cut's in/out and not the parent's.
                result = m_producer.get_int(kFilterOutProperty);
            } else {
                result = m_producer.get_out();
            }
        } else {
            result = m_filter.get_int("out");
        }
    }
    return result;
}

void QmlFilter::setOut(int value)
{
    set("out", value);
}

int QmlFilter::animateIn()
{
    return m_filter.time_to_frames(m_filter.get(kShotcutAnimInProperty));
}

void QmlFilter::setAnimateIn(int value)
{
    m_filter.set(kShotcutAnimInProperty, m_filter.frames_to_time(qBound(0, value, duration()), mlt_time_clock));
    emit animateInChanged();
}

int QmlFilter::animateOut()
{
    return m_filter.time_to_frames(m_filter.get(kShotcutAnimOutProperty));
}

void QmlFilter::setAnimateOut(int value)
{
    m_filter.set(kShotcutAnimOutProperty, m_filter.frames_to_time(qBound(0, value, duration()), mlt_time_clock));
    emit animateOutChanged();
}

int QmlFilter::duration()
{
    return out() - in() + 1;
}

Mlt::Animation QmlFilter::getAnimation(const QString& name)
{
    if (m_filter.is_valid()) {
        const char* propertyName = name.toUtf8().constData();
        if (!m_filter.get_animation(propertyName)) {
            // Cause a string property to be interpreted as animated value.
            m_filter.anim_get_double(propertyName, 0, duration());
        }
        return m_filter.get_animation(propertyName);
    }
    return Mlt::Animation();
}

int QmlFilter::keyframeCount(const QString& name)
{
    return getAnimation(name).key_count();
}

void QmlFilter::resetProperty(const QString& name)
{
    m_filter.clear(name.toUtf8().constData());
}

void QmlFilter::clearSimpleAnimation(const QString& name)
{
    // Reset the animation if there are no keyframes yet.
    if (animateIn() <= 0 && animateOut() <= 0 && keyframeCount(name) <= 0)
        resetProperty(name);
    setAnimateIn(0);
    setAnimateOut(0);
}

void QmlFilter::preset(const QString &name)
{
    if (!m_filter.is_valid()) return;
    QDir dir(Settings.appDataLocation());

    if (!dir.cd("presets") || !dir.cd(objectNameOrService()))
        return;

#if LIBMLT_VERSION_INT >= ((6<<16)+(9<<8))
    // Detect the preset file format
    bool isYaml = false;
    QFile presetFile(dir.filePath(name));
    if(presetFile.open(QIODevice::ReadOnly)) {
        if(presetFile.readLine(4) == "---") {
            isYaml = true;
        }
        presetFile.close();
    }

    if(isYaml) {
        // Load from YAML file.
        QScopedPointer<Mlt::Properties> properties(Mlt::Properties::parse_yaml(dir.filePath(name).toUtf8().constData()));
        m_filter.inherit(*properties);
    } else {
        // Load from legacy preset file
        m_filter.load(dir.filePath(name).toUtf8().constData());
    }
#else
    m_filter.load(dir.filePath(name).toUtf8().constData());
#endif

    emit changed();
}

QString QmlFilter::objectNameOrService()
{
    return m_metadata->objectName().isEmpty()? m_metadata->mlt_service() : m_metadata->objectName();
}

int QmlFilter::keyframeIndex(Mlt::Animation& animation, int position)
{
    int result = -1;
    if (animation.is_valid()) {
        for (int i = 0; i < animation.key_count() && result == -1; i++) {
            int frame = animation.key_get_frame(i);
            if (frame == position)
                result = i;
            else if (frame > position)
                break;
        }
    }
    return result;
}

mlt_keyframe_type QmlFilter::getKeyframeType(Mlt::Animation& animation, int position, mlt_keyframe_type defaultType)
{
    mlt_keyframe_type result = mlt_keyframe_linear;
    if (animation.is_valid()) {
        mlt_keyframe_type existingType = defaultType;
        if (animation.is_key(position)) {
            existingType = animation.key_get_type(keyframeIndex(animation, position));
        } else if (defaultType < 0) {
            int previous = animation.previous_key(position);
            if (previous >= 0)
                existingType = animation.keyframe_type(previous);
        }
        if (existingType >= 0)
            result = existingType;
    }
    return result;
}

AnalyzeDelegate::AnalyzeDelegate(Mlt::Filter& filter)
    : QObject(0)
    , m_filter(filter)
{}

void AnalyzeDelegate::onAnalyzeFinished(AbstractJob *job, bool isSuccess)
{
    QString fileName = job->objectName();

    if (isSuccess) {
        // parse the xml
        QFile file(fileName);
        file.open(QIODevice::ReadOnly);
        QDomDocument dom(fileName);
        dom.setContent(&file);
        file.close();

        QDomNodeList filters = dom.elementsByTagName("filter");
        for (int i = 0; i < filters.size(); i++) {
            QDomNode filterNode = filters.at(i);
            bool found = false;

            QDomNodeList properties = filterNode.toElement().elementsByTagName("property");
            for (int j = 0; j < properties.size(); j++) {
                QDomNode propertyNode = properties.at(j);
                if (propertyNode.attributes().namedItem("name").toAttr().value() == "mlt_service"
                        && propertyNode.toElement().text() == m_filter.get("mlt_service")) {
                    found = true;
                    break;
                }
            }
            if (found) {
                for (int j = 0; j < properties.size(); j++) {
                    QDomNode propertyNode = properties.at(j);
                    if (propertyNode.attributes().namedItem("name").toAttr().value() == "results") {
                        m_filter.set("results", propertyNode.toElement().text().toUtf8().constData());
                        emit MAIN.filterController()->attachedModel()->changed();
                    }
                }
                break;
            }
        }
    }
    QFile::remove(fileName);
    deleteLater();
}
