/*
 * Copyright (c) 2013-2020 Meltytech, LLC
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
#include "jobs/encodejob.h"
#include "shotcut_mlt_properties.h"
#include "settings.h"
#include "util.h"
#include "proxymanager.h"
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
        if (position < 0)
            return QString::fromUtf8(m_filter.get(qUtf8Printable(name)));
        else
            return QString::fromUtf8(m_filter.anim_get(qUtf8Printable(name), position, duration()));
    } else {
        return QString();
    }
}

double QmlFilter::getDouble(QString name, int position)
{
    if (m_filter.is_valid()) {
        if (position < 0)
            return m_filter.get_double(qUtf8Printable(name));
        else
            return m_filter.anim_get_double(qUtf8Printable(name), position, duration());
    } else {
        return 0.0;
    }
}

QRectF QmlFilter::getRect(QString name, int position)
{
    if (!m_filter.is_valid()) return QRectF();
    const char* s = m_filter.get(qUtf8Printable(name));
    if (s) {
        mlt_rect rect;
        if (position < 0) {
            rect = m_filter.get_rect(qUtf8Printable(name));
        } else {
            rect = m_filter.anim_get_rect(qUtf8Printable(name), position, duration());
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

QStringList QmlFilter::getGradient(QString name)
{
    QStringList list;
    for (int i = 1; i <= 10; i++) {
        QString colorName = name + "." + QString::number(i);
        const char* value = m_filter.get(qUtf8Printable(colorName));
        if (value) {
            list.append(QString::fromUtf8(value));
        } else {
            break;
        }
    }
    return list;
}

void QmlFilter::set(QString name, QString value, int position)
{
    if (!m_filter.is_valid()) return;
    if (position < 0) {
        if (qstrcmp(m_filter.get(qUtf8Printable(name)), qUtf8Printable(value)))  {
            m_filter.set_string(qUtf8Printable(name), qUtf8Printable(value)) ;
            emit changed(name);
        }
    } else {
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_filter.get_animation(qUtf8Printable(name)));
        if (!animation.is_valid() || !animation.is_key(position)
                || value != m_filter.anim_get(qUtf8Printable(name), position, duration())) {
            m_filter.anim_set(qUtf8Printable(name), qUtf8Printable(value), position, duration());
            emit changed(name);
        }
    }
}

void QmlFilter::set(QString name, double value, int position, mlt_keyframe_type keyframeType)
{
    if (!m_filter.is_valid()) return;
    if (position < 0) {
        if (!m_filter.get(qUtf8Printable(name))
            || m_filter.get_double(qUtf8Printable(name)) != value) {
            double delta = value - m_filter.get_double(qUtf8Printable(name));
            m_filter.set(qUtf8Printable(name), value);
            emit changed(name);
            if (name == "in") {
                emit inChanged(delta);
            } else if (name == "out") {
                emit outChanged(delta);
            }
        }
    } else {
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_filter.get_animation(qUtf8Printable(name)));
        if (!animation.is_valid() || !animation.is_key(position)
                || value != m_filter.anim_get_double(qUtf8Printable(name), position, duration())) {
            mlt_keyframe_type type = getKeyframeType(animation, position, keyframeType);
            m_filter.anim_set(qUtf8Printable(name), value, position, duration(), type);
            emit changed(name);
        }
    }
}

void QmlFilter::set(QString name, int value, int position, mlt_keyframe_type keyframeType)
{
    if (!m_filter.is_valid()) return;
    if (position < 0) {
        if (!m_filter.get(qUtf8Printable(name))
            || m_filter.get_int(qUtf8Printable(name)) != value) {
            int delta = value - m_filter.get_int(qUtf8Printable(name));
            m_filter.set(qUtf8Printable(name), value);
            emit changed(name);
            if (name == "in") {
                emit inChanged(delta);
            } else if (name == "out") {
                emit outChanged(delta);
            }
        }
    } else {
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_filter.get_animation(qUtf8Printable(name)));
        if (!animation.is_valid() || !animation.is_key(position)
                || value != m_filter.anim_get_int(qUtf8Printable(name), position, duration())) {
            mlt_keyframe_type type = getKeyframeType(animation, position, keyframeType);
            m_filter.anim_set(qUtf8Printable(name), value, position, duration(), type);
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
        mlt_rect rect = m_filter.get_rect(qUtf8Printable(name));
        if (!m_filter.get(qUtf8Printable(name)) || x != rect.x || y != rect.y
            || width != rect.w || height != rect.h || opacity != rect.o) {
            m_filter.set(qUtf8Printable(name), x, y, width, height, opacity);
            emit changed(name);
        }
    } else {
        mlt_rect rect = m_filter.anim_get_rect(qUtf8Printable(name), position, duration());
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_filter.get_animation(qUtf8Printable(name)));
        if (!animation.is_valid() || !animation.is_key(position)
                || x != rect.x || y != rect.y || width != rect.w || height != rect.h || opacity != rect.o) {
            rect.x = x;
            rect.y = y;
            rect.w = width;
            rect.h = height;
            rect.o = opacity;
            mlt_keyframe_type type = getKeyframeType(animation, position, keyframeType);
            m_filter.anim_set(qUtf8Printable(name), rect, position, duration(), type);
            emit changed(name);
        }
    }
}

void QmlFilter::setGradient(QString name, const QStringList& gradient)
{
    for (int i = 1; i <= 10;  i++) {
        QString colorName = name + "." + QString::number(i);
        if (i <= gradient.length()) {
            m_filter.set(qUtf8Printable(colorName), qUtf8Printable(gradient[i-1]));
        } else {
            m_filter.clear(qUtf8Printable(colorName));
        }
    }
    emit changed();
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
                for (auto& s : dir.entryList(QDir::Files | QDir::Readable)) {
                    if (s == QUrl::toPercentEncoding(QUrl::fromPercentEncoding(s.toUtf8())))
                        m_presets << QUrl::fromPercentEncoding(s.toUtf8());
                    else
                        m_presets << s;
                }
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
    QString preset = name.isEmpty()? tr("(defaults)") : QString::fromUtf8(QUrl::toPercentEncoding(name));
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
    loadPresets();
    return m_presets.indexOf(name);
}

void QmlFilter::deletePreset(const QString &name)
{
    QDir dir(Settings.appDataLocation());
    if (dir.cd("presets") && dir.cd(objectNameOrService())) {
        if (!QFile(dir.filePath(QUrl::toPercentEncoding(name))).remove())
            QFile(dir.filePath(name)).remove();
    }
    m_presets.removeOne(name);
    emit presetsChanged();
}

void QmlFilter::analyze(bool isAudio)
{
    Mlt::Service service(mlt_service(m_filter.get_data("service")));

    // get temp file for input xml
    QString filename(m_filter.get("filename"));
    QScopedPointer<QTemporaryFile> tmp(Util::writableTemporaryFile(filename));
    tmp->open();

    m_filter.set("results", nullptr, 0);
    int disable = m_filter.get_int("disable");
    m_filter.set("disable", 0);
    if (!isAudio) m_filter.set("analyze", 1);

    // Fix in/out points of filters on clip-only project.
    if (MLT.isSeekableClip()) {
        Mlt::Producer producer(MLT.profile(), "xml-string", MLT.XML().toUtf8().constData());
        service = Mlt::Service(producer);
        int producerIn = producer.get_in();
        if (producerIn > 0) {
            int n = producer.filter_count();
            for (int i = 0; i < n; i++) {
                Mlt::Filter filter(*producer.filter(i));
                if (filter.get_in() > 0)
                    filter.set_in_and_out(filter.get_in() - producerIn, filter.get_out() - producerIn);
            }
        }
    }

    MLT.saveXML(tmp->fileName(), &service, false /* without relative paths */, tmp.data());
    tmp->close();

    if (!isAudio) m_filter.set("analyze", 0);
    m_filter.set("disable", disable);

    // get temp filename for output xml
    QScopedPointer<QTemporaryFile> tmpTarget(Util::writableTemporaryFile(filename));
    tmpTarget->open();
    tmpTarget->close();

    // parse xml
    QFile f1(tmp->fileName());
    f1.open(QIODevice::ReadOnly);
    QDomDocument dom(tmp->fileName());
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
    consumerNode.setAttribute("resource", tmpTarget->fileName());

    AbstractJob* job = new MeltJob(tmpTarget->fileName(), dom.toString(2),
        MLT.profile().frame_rate_num(), MLT.profile().frame_rate_den());
    if (job) {
        AnalyzeDelegate* delegate = new AnalyzeDelegate(m_filter);
        connect(job, &AbstractJob::finished, delegate, &AnalyzeDelegate::onAnalyzeFinished);
        connect(job, &AbstractJob::finished, this, &QmlFilter::analyzeFinished);
        job->setLabel(tr("Analyze %1").arg(Util::baseName(ProxyManager::resource(service))));

        // Touch the target .stab file. This prevents multiple jobs from trying
        // to write the same file.
        if (!filename.isEmpty() && !QFile::exists(filename)) {
            job->setProperty("filename", filename);
            QFile file(filename);
            file.open(QFile::WriteOnly);
            file.write("");
        }

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
        Util::getHash(m_filter);
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
        if (!m_filter.get_animation(qUtf8Printable(name))) {
            // Cause a string property to be interpreted as animated value.
            m_filter.anim_get_double(qUtf8Printable(name), 0, duration());
        }
        return m_filter.get_animation(qUtf8Printable(name));
    }
    return Mlt::Animation();
}

int QmlFilter::keyframeCount(const QString& name)
{
    return getAnimation(name).key_count();
}

void QmlFilter::resetProperty(const QString& name)
{
    m_filter.clear(qUtf8Printable(name));
    emit changed();
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

    auto fileName = dir.filePath(QUrl::toPercentEncoding(name));

    // Detect the preset file format
    bool isYaml = false;
    QFile presetFile(fileName);
    if (presetFile.open(QIODevice::ReadOnly)) {
        if(presetFile.readLine(4) == "---") {
            isYaml = true;
        }
        presetFile.close();
    } else {
        presetFile.setFileName(dir.filePath(name));
        if (presetFile.open(QIODevice::ReadOnly)) {
            fileName = dir.filePath(name);
            if(presetFile.readLine(4) == "---") {
                isYaml = true;
            }
            presetFile.close();
        }
    }

    if(isYaml) {
        // Load from YAML file.
        QScopedPointer<Mlt::Properties> properties(Mlt::Properties::parse_yaml(qUtf8Printable(fileName)));
        if (properties && properties->is_valid()) {
            QChar decimalPoint = MLT.decimalPoint();
            for (int i = 0; i < properties->count(); i++) {
                QString name(properties->get_name(i));
                if (m_metadata->mlt_service() == "dynamictext" && name == "argument")
                    continue;
                // Convert numeric strings to the current MLT numeric locale.
                QString value = QString::fromUtf8(properties->get(i));
                if (Util::convertDecimalPoints(value, decimalPoint))
                    properties->set(qUtf8Printable(name), qUtf8Printable(value));
            }
            m_filter.inherit(*properties);
        }
    } else {
        // Load from legacy preset file
        m_filter.load(qUtf8Printable(fileName));
    }

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

bool QmlFilter::isAtLeastVersion(const QString& version)
{
    QVersionNumber v1 = QVersionNumber::fromString(version);
    QVersionNumber v2 = QVersionNumber::fromString(m_metadata->property("version").toString());
    return v2 >= v1;
}

void QmlFilter::deselect()
{
    MAIN.filterController()->setCurrentFilter(-2);
}

AnalyzeDelegate::AnalyzeDelegate(Mlt::Filter& filter)
    : QObject(0)
    , m_uuid(QUuid::createUuid())
    , m_serviceName(filter.get("mlt_service"))
{
    filter.set(kShotcutHashProperty, m_uuid.toByteArray().data());
}

class FindFilterParser : public Mlt::Parser
{
private:
    QUuid m_uuid;
    QList<Mlt::Filter> m_filters;

public:
    FindFilterParser(QUuid uuid)
        : Mlt::Parser()
        , m_uuid(uuid)
    {}
    
    QList<Mlt::Filter>& filters() { return m_filters; }
    
    int on_start_filter(Mlt::Filter *filter) {
        QByteArray uuid = filter->get(kShotcutHashProperty);
        if (uuid == m_uuid.toByteArray())
            m_filters << Mlt::Filter(*filter);
        return 0;
    }
    int on_start_producer(Mlt::Producer*) { return 0; }
    int on_end_producer(Mlt::Producer*) { return 0; }
    int on_start_playlist(Mlt::Playlist*) { return 0; }
    int on_end_playlist(Mlt::Playlist*) { return 0; }
    int on_start_tractor(Mlt::Tractor*) { return 0; }
    int on_end_tractor(Mlt::Tractor*) { return 0; }
    int on_start_multitrack(Mlt::Multitrack*) { return 0; }
    int on_end_multitrack(Mlt::Multitrack*) { return 0; }
    int on_start_track() { return 0; }
    int on_end_track() { return 0; }
    int on_end_filter(Mlt::Filter*) { return 0; }
    int on_start_transition(Mlt::Transition*) { return 0; }
    int on_end_transition(Mlt::Transition*) { return 0; }
};

void AnalyzeDelegate::updateJob(EncodeJob* job, const QString& results)
{
    bool isUpdated = false;

    // parse the xml
    QFile file(job->xmlPath());
    file.open(QIODevice::ReadOnly);
    QDomDocument dom(job->xmlPath());
    dom.setContent(&file);
    file.close();

    // look for the matching filter elements
    QDomNodeList filters = dom.elementsByTagName("filter");
    for (int i = 0; i < filters.size(); i++) {
        QDomNode filterNode = filters.at(i);
        bool found = false;

        QDomNodeList properties = filterNode.toElement().elementsByTagName("property");
        for (int j = 0; j < properties.size(); j++) {
            QDomNode propertyNode = properties.at(j);
            if (propertyNode.attributes().namedItem("name").toAttr().value() == kShotcutHashProperty
                    && propertyNode.toElement().text() == m_uuid.toString()) {
                // found a matching filter
                found = true;
                break;
            }
        }
        if (found) {
            // remove existing results if any
            for (int j = 0; j < properties.size(); j++) {
                QDomNode propertyNode = properties.at(j);
                if (propertyNode.attributes().namedItem("name").toAttr().value() == "results") {
                    filterNode.removeChild(propertyNode);
                    break;
                }
            }
            // add the results
            QDomText textNode = dom.createTextNode(results);
            QDomElement propertyNode = dom.createElement("property");
            propertyNode.setAttribute("name", "results");
            propertyNode.appendChild(textNode);
            filterNode.appendChild(propertyNode);
            isUpdated = true;
            LOG_INFO() << "updated pending job" << job->label() << "with results:" << results;
        }
    }
    
    if (isUpdated) {
        // Save the new XML.
        file.open(QIODevice::WriteOnly);
        QTextStream textStream(&file);
        dom.save(textStream, 2);
        file.close();
    }
}

void AnalyzeDelegate::onAnalyzeFinished(AbstractJob *job, bool isSuccess)
{
    QString fileName = job->objectName();

    if (isSuccess) {
        QString results = resultsFromXml(fileName, m_serviceName);
        if (!results.isEmpty()) {
            // look for filters by UUID in each pending export job.
            foreach (AbstractJob* job, JOBS.jobs()) {
                if (!job->ran() && typeid(*job) == typeid(EncodeJob)) {
                    updateJob(dynamic_cast<EncodeJob*>(job), results);
                }
            }
    
            // Locate filters in memory by UUID.
            FindFilterParser graphParser(m_uuid);
            if (MAIN.isMultitrackValid()) {
                graphParser.start(*MAIN.multitrack());
                foreach (Mlt::Filter filter, graphParser.filters())
                    updateFilter(filter, results);
            }
            if (MAIN.playlist() && MAIN.playlist()->count() > 0) {
                graphParser.start(*MAIN.playlist());
                foreach (Mlt::Filter filter, graphParser.filters())
                    updateFilter(filter, results);
            }
            if (MLT.producer() && MLT.producer()->is_valid()) {
                graphParser.start(*MLT.producer());
                foreach (Mlt::Filter filter, graphParser.filters())
                    updateFilter(filter, results);
            }
            if (MLT.savedProducer() && MLT.savedProducer()->is_valid()) {
                graphParser.start(*MLT.savedProducer());
                foreach (Mlt::Filter filter, graphParser.filters())
                    updateFilter(filter, results);
            }
            emit MAIN.filterController()->attachedModel()->changed();
        }
    } else if (!job->property("filename").isNull()) {
        QFile file(job->property("filename").toString());
        if (file.exists() && file.size() == 0)
            file.remove();
    }
    QFile::remove(fileName);
    deleteLater();
}

QString AnalyzeDelegate::resultsFromXml(const QString& fileName, const QString& serviceName)
{
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
                    && propertyNode.toElement().text() == serviceName) {
                found = true;
                break;
            }
        }
        if (found) {
            for (int j = 0; j < properties.size(); j++) {
                QDomNode propertyNode = properties.at(j);
                if (propertyNode.attributes().namedItem("name").toAttr().value() == "results") {
                    return propertyNode.toElement().text();
                }
            }
            break;
        }
    }
    return QString();
}

void AnalyzeDelegate::updateFilter(Mlt::Filter& filter, const QString& results)
{
    filter.set("results", qUtf8Printable(results));
    filter.set("reload", 1);
    filter.clear(kShotcutHashProperty);
    LOG_INFO() << "updated filter" << filter.get("mlt_service") << "with results:" << results;
}
