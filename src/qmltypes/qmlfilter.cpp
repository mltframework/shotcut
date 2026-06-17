/*
 * Copyright (c) 2013-2026 Meltytech, LLC
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

#include "Logger.h"
#include "commands/filtercommands.h"
#include "controllers/filtercontroller.h"
#include "jobqueue.h"
#include "jobs/encodejob.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "proxymanager.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <MltProducer.h>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIODevice>
#include <QTemporaryFile>
#include <QtXml>

/*!
    \qmltype Filter
    \inqmlmodule org.shotcut.qml
    \brief Provides access to the current filter's properties, keyframes, and presets.

    \c Filter is the primary scripting interface for filter UI panels and VUI overlays.
    Each filter's QML file receives a \c filter context property of this type.
    Use it to read and write MLT service properties, manage keyframes,
    load/save presets, and trigger analysis jobs.

    \code
    // Inside a filter's QML panel:
    filter.set("level", slider.value)
    var current = filter.getDouble("level")
    filter.startUndoParameterCommand("Change Level")
    filter.set("level", newValue)
    filter.endUndoCommand()
    \endcode
*/

/*!
    \qmlsignal Filter::presetsChanged()
    \brief Emitted when the list of saved presets changes.
*/

/*!
    \qmlsignal Filter::analyzeFinished(bool isSuccess)
    \brief Emitted when an analysis job completes. \a isSuccess is \c true on success.
*/

/*!
    \qmlsignal Filter::changed(string name)
    \brief Emitted when any filter property changes. \a name is the property name,
    or empty if multiple properties changed.
*/

/*!
    \qmlsignal Filter::inChanged(int delta)
    \brief Emitted when the filter's in-point changes. \a delta is the frame offset.
*/

/*!
    \qmlsignal Filter::outChanged(int delta)
    \brief Emitted when the filter's out-point changes. \a delta is the frame offset.
*/

/*!
    \qmlsignal Filter::animateInChanged()
    \brief Emitted when \l animateIn changes.
*/

/*!
    \qmlsignal Filter::animateOutChanged()
    \brief Emitted when \l animateOut changes.
*/

/*!
    \qmlsignal Filter::animateInOutChanged()
    \brief Emitted when either \l animateIn or \l animateOut changes.
*/

/*!
    \qmlsignal Filter::durationChanged()
    \brief Emitted when the filter's \l duration changes.
*/

/*!
    \qmlsignal Filter::propertyChanged(string name)
    \brief Emitted when a specific named property \a name changes. Use in bindings that
    need to react to individual property updates.
*/

/*!
    \qmlproperty bool Filter::isNew
    \brief Whether this filter was just added or its UI is simply being reloaded.
    Use this to set sensible initial property values on first load.
*/

/*!
    \qmlproperty string Filter::path
    \brief Absolute path to the directory containing this filter's QML files.
    Use to load auxiliary QML files relative to the filter.
*/

/*!
    \qmlproperty list<string> Filter::presets
    \brief The list of saved preset names for this filter.
    Notifies \l presetsChanged when the list changes.
*/

/*!
    \qmlproperty bool Filter::blockSignals
    \brief When \c true, suppresses all QML signals from this object.
    Set while performing bulk property updates to avoid redundant UI refreshes.
*/

QmlFilter::QmlFilter()
    : QObject(nullptr)
    , m_metadata(nullptr)
    , m_service(mlt_service(nullptr))
    , m_producer(mlt_producer(nullptr))
    , m_isNew(false)
    , m_changeInProgress(0)
{
    connect(this, SIGNAL(inChanged(int)), this, SIGNAL(durationChanged()));
    connect(this, SIGNAL(outChanged(int)), this, SIGNAL(durationChanged()));
}

QmlFilter::QmlFilter(Mlt::Service &mltService, const QmlMetadata *metadata, QObject *parent)
    : QObject(parent)
    , m_metadata(metadata)
    , m_service(mltService)
    , m_producer(mlt_producer(nullptr))
    , m_path(m_metadata->path().absolutePath().append('/'))
    , m_isNew(false)
    , m_changeInProgress(false)
{
    if (m_service.type() == mlt_service_filter_type) {
        // Every attached filter has a service property that points to the service to which it is attached.
        m_producer = Mlt::Producer(
            mlt_producer(m_service.is_valid() ? m_service.get_data("service") : 0));
    } else if (m_service.type() == mlt_service_link_type) {
        // Every attached link has a chain property that points to the chain to which it is attached.
        m_producer = Mlt::Producer(
            mlt_producer(m_service.is_valid() ? m_service.get_data("chain") : 0));
    }
}

QmlFilter::~QmlFilter() {}

/*!
    \qmlmethod string Filter::get(string name, int position = -1)
    \brief Returns the value of MLT property \a name as a string.
    If time \a position is \c -1 (default), returns the current value;
    otherwise returns the interpolated value at that frame position.
    To get an integer value, convert the result using \c parseInt(), e.g. \c parseInt(filter.get("someProperty")).
*/

QString QmlFilter::get(QString name, int position)
{
    if (m_service.is_valid()) {
        if (position < 0)
            return QString::fromUtf8(m_service.get(qUtf8Printable(name)));
        else
            return QString::fromUtf8(m_service.anim_get(qUtf8Printable(name), position, duration()));
    } else {
        return QString();
    }
}

/*!
    \qmlmethod color Filter::getColor(string name, int position = -1)
    \brief Returns the value of MLT property \a name as a \c color.
    \a position defaults to \c -1 (current value).
*/

QColor QmlFilter::getColor(QString name, int position)
{
    mlt_color color = {0, 0, 0, 0};
    if (m_service.is_valid()) {
        if (position < 0)
            color = m_service.get_color(qUtf8Printable(name));
        else
            color = m_service.anim_get_color(qUtf8Printable(name), position, duration());
    }
    return QColor(color.r, color.g, color.b, color.a);
}

/*!
    \qmlmethod real Filter::getDouble(string name, int position = -1)
    \brief Returns the value of MLT property \a name as a floating-point number.
    \a position defaults to \c -1 (current value).
*/

double QmlFilter::getDouble(QString name, int position)
{
    if (m_service.is_valid()) {
        if (position < 0)
            return m_service.get_double(qUtf8Printable(name));
        else
            return m_service.anim_get_double(qUtf8Printable(name), position, duration());
    } else {
        return 0.0;
    }
}

/*!
    \qmlmethod rect Filter::getRect(string name, int position = -1)
    \brief Returns the value of MLT property \a name as a \c rect.
    The rectangle values are in the project coordinate space.
    \a position defaults to \c -1 (current value).
*/

QRectF QmlFilter::getRect(QString name, int position)
{
    if (!m_service.is_valid())
        return QRectF();
    const char *s = m_service.get(qUtf8Printable(name));
    if (s) {
        mlt_rect rect;
        if (position < 0) {
            rect = m_service.get_rect(qUtf8Printable(name));
        } else {
            rect = m_service.anim_get_rect(qUtf8Printable(name), position, duration());
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

/*!
    \qmlmethod Filter::removeRectPercents(string name)
    \brief Converts a rect property stored as percentages to absolute pixel values.
    \a name is the MLT property to convert in-place.
*/

void QmlFilter::removeRectPercents(QString name)
{
    // This method iterates over each keyframe and converts the percentage values to absolute.
    if (!m_service.is_valid())
        return;
    const char *s = m_service.get(qUtf8Printable(name));
    if (s && ::strchr(s, '%')) {
        m_service.anim_get_rect(qUtf8Printable(name), 0, duration());
        auto anim = m_service.get_anim(qUtf8Printable(name));
        if (anim && anim->is_valid()) {
            mlt_rect rect;
            for (int i = 0; i < anim->key_count(); ++i) {
                int position;
                mlt_keyframe_type keyType;
                anim->key_get(i, position, keyType);
                rect = m_service.anim_get_rect(qUtf8Printable(name), position, duration());
                auto r = QRectF(qRound(rect.x * MLT.profile().width()),
                                qRound(rect.y * MLT.profile().height()),
                                qRound(rect.w * MLT.profile().width()),
                                qRound(rect.h * MLT.profile().height()));
                LOG_DEBUG() << r << position;
                set(name, r.x(), r.y(), r.width(), r.height(), 1.0, position, keyType);
                LOG_DEBUG() << m_service.get(qUtf8Printable(name));
            }
        }
    }
}

/*!
    \qmlmethod list<string> Filter::getGradient(string name)
    \brief Returns the gradient stop colors for property \a name as a list of color strings.
*/

QStringList QmlFilter::getGradient(QString name)
{
    QStringList list;
    for (int i = 1; i <= 10; i++) {
        QString colorName = name + "." + QString::number(i);
        const char *value = m_service.get(qUtf8Printable(colorName));
        if (value) {
            list.append(QString::fromUtf8(value));
        } else {
            break;
        }
    }
    return list;
}

/*!
    \qmlmethod Filter::set(string name, string value, int position = -1)
    \brief Sets MLT property \a name to the string \a value.
    If time \a position is given, sets a keyframe at that position.
*/

/*!
    \qmlmethod Filter::set(string name, color value, int position = -1, int keyframeType = -1)
    \brief Sets MLT property \a name to the color \a value, optionally at time \a position.
    \a keyframeType is one of the \c mlt_keyframe_type enum values; use \c -1 for the default.
*/

/*!
    \qmlmethod Filter::set(string name, real value, int position = -1, int keyframeType = -1)
    \brief Sets MLT property \a name to the floating-point \a value, optionally at time \a position.
*/

/*!
    \qmlmethod Filter::set(string name, int value, int position = -1, int keyframeType = -1)
    \brief Sets MLT property \a name to the integer \a value, optionally at time \a position.
*/

/*!
    \qmlmethod Filter::set(string name, bool value, int position = -1, int keyframeType = -1)
    \brief Sets MLT property \a name to the boolean \a value, optionally at time \a position.
*/

/*!
    \qmlmethod Filter::set(string name, real x, real y, real width, real height, real opacity = 1.0, int position = -1, int keyframeType = -1)
    \brief Sets MLT property \a name to a rect specified by components, optionally at time \a position.
    \a opacity is clamped to [0.0, 1.0].
*/

/*!
    \qmlmethod Filter::set(string name, rect value, int position = -1, int keyframeType = -1)
    \brief Sets MLT property \a name to the \a value rect, optionally at time \a position.
*/

void QmlFilter::set(QString name, QString value, int position)
{
    if (!m_service.is_valid())
        return;
    if (position < 0) {
        if (qstrcmp(m_service.get(qUtf8Printable(name)), qUtf8Printable(value))) {
            m_service.set_string(qUtf8Printable(name), qUtf8Printable(value));
            emit changed(name);
            updateUndoCommand(name);
        }
    } else {
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_service.get_animation(qUtf8Printable(name)));
        if (!animation.is_valid() || !animation.is_key(position)
            || value != m_service.anim_get(qUtf8Printable(name), position, duration())) {
            m_service.anim_set(qUtf8Printable(name), qUtf8Printable(value), position, duration());
            emit changed(name);
            updateUndoCommand(name);
        }
    }
}

void QmlFilter::set(QString name, const QColor &value, int position, mlt_keyframe_type keyframeType)
{
    if (!m_service.is_valid())
        return;
    if (position < 0) {
        auto mltColor = m_service.get_color(qUtf8Printable(name));
        if (!m_service.get(qUtf8Printable(name))
            || value != QColor(mltColor.r, mltColor.g, mltColor.b, mltColor.a)) {
            m_service.set(qUtf8Printable(name), Util::mltColorFromQColor(value));
            emit changed(name);
            updateUndoCommand(name);
        }
    } else {
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_service.get_animation(qUtf8Printable(name)));
        auto mltColor = m_service.anim_get_color(qUtf8Printable(name), position, duration());
        if (!animation.is_valid() || !animation.is_key(position)
            || value != QColor(mltColor.r, mltColor.g, mltColor.b, mltColor.a)) {
            m_service.anim_set(qUtf8Printable(name),
                               Util::mltColorFromQColor(value),
                               position,
                               duration());
            emit changed(name);
            updateUndoCommand(name);
        }
    }
}

void QmlFilter::set(QString name, double value, int position, mlt_keyframe_type keyframeType)
{
    if (!m_service.is_valid())
        return;
    if (position < 0) {
        if (!m_service.get(qUtf8Printable(name))
            || m_service.get_double(qUtf8Printable(name)) != value) {
            double delta = value - m_service.get_double(qUtf8Printable(name));
            m_service.set(qUtf8Printable(name), value);
            emit changed(name);
            if (name == "in") {
                emit inChanged(delta);
            } else if (name == "out") {
                emit outChanged(delta);
            }
            updateUndoCommand(name);
        }
    } else {
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_service.get_animation(qUtf8Printable(name)));
        if (!animation.is_valid() || !animation.is_key(position)
            || value != m_service.anim_get_double(qUtf8Printable(name), position, duration())) {
            mlt_keyframe_type type = getKeyframeType(animation, position, keyframeType);
            m_service.anim_set(qUtf8Printable(name), value, position, duration(), type);
            emit changed(name);
            updateUndoCommand(name);
        }
    }
}

void QmlFilter::set(QString name, int value, int position, mlt_keyframe_type keyframeType)
{
    if (!m_service.is_valid())
        return;
    if (position < 0) {
        if (!m_service.get(qUtf8Printable(name))
            || m_service.get_int(qUtf8Printable(name)) != value) {
            int delta = value - m_service.get_int(qUtf8Printable(name));
            m_service.set(qUtf8Printable(name), value);
            emit changed(name);
            if (name == "in") {
                emit inChanged(delta);
            } else if (name == "out") {
                emit outChanged(delta);
            }
            updateUndoCommand(name);
        }
    } else {
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_service.get_animation(qUtf8Printable(name)));
        if (!animation.is_valid() || !animation.is_key(position)
            || value != m_service.anim_get_int(qUtf8Printable(name), position, duration())) {
            mlt_keyframe_type type = getKeyframeType(animation, position, keyframeType);
            m_service.anim_set(qUtf8Printable(name), value, position, duration(), type);
            emit changed(name);
            updateUndoCommand(name);
        }
    }
}

void QmlFilter::set(QString name, bool value, int position, mlt_keyframe_type keyframeType)
{
    set(name, value ? 1 : 0, position, keyframeType);
}

void QmlFilter::set(QString name,
                    double x,
                    double y,
                    double width,
                    double height,
                    double opacity,
                    int position,
                    mlt_keyframe_type keyframeType)
{
    if (!m_service.is_valid())
        return;
    if (position < 0) {
        mlt_rect rect = m_service.get_rect(qUtf8Printable(name));
        if (!m_service.get(qUtf8Printable(name)) || x != rect.x || y != rect.y || width != rect.w
            || height != rect.h || opacity != rect.o) {
            m_service.set(qUtf8Printable(name), x, y, width, height, opacity);
            emit changed(name);
            updateUndoCommand(name);
        }
    } else {
        mlt_rect rect = m_service.anim_get_rect(qUtf8Printable(name), position, duration());
        // Only set an animation keyframe if it does not already exist with the same value.
        Mlt::Animation animation(m_service.get_animation(qUtf8Printable(name)));
        if (!animation.is_valid() || !animation.is_key(position) || x != rect.x || y != rect.y
            || width != rect.w || height != rect.h || opacity != rect.o) {
            if (animation.key_count() < 1) {
                // Clear the string value when setting animation for the first time
                m_service.clear(qUtf8Printable(name));
            }
            rect.x = x;
            rect.y = y;
            rect.w = width;
            rect.h = height;
            rect.o = opacity;
            mlt_keyframe_type type = getKeyframeType(animation, position, keyframeType);
            m_service.anim_set(qUtf8Printable(name), rect, position, duration(), type);
            emit changed(name);
            updateUndoCommand(name);
        }
    }
}

/*!
    \qmlmethod Filter::setGradient(string name, list<string> colors)
    \brief Sets the gradient stops for property \a name from a list of \a colors strings.
*/

void QmlFilter::setGradient(QString name, const QStringList &gradient)
{
    for (int i = 1; i <= 10; i++) {
        QString colorName = name + "." + QString::number(i);
        if (i <= gradient.length()) {
            m_service.set(qUtf8Printable(colorName), qUtf8Printable(gradient[i - 1]));
        } else {
            m_service.clear(qUtf8Printable(colorName));
        }
    }
    emit changed(name.toUtf8().constData());
    updateUndoCommand(name);
}

void QmlFilter::set(QString name, const QRectF &rect, int position, mlt_keyframe_type keyframeType)
{
    set(name, rect.x(), rect.y(), rect.width(), rect.height(), 1.0, position, keyframeType);
}

/*!
    \qmlmethod Filter::loadPresets()
    \brief Reloads the list of saved presets from disk and emits \l presetsChanged.
*/

void QmlFilter::loadPresets()
{
    m_presets.clear();
    QDir dir(Settings.appDataLocation());
    if (dir.cd("presets")) {
        QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Executable);
        foreach (QString s, entries) {
            if (s == objectNameOrService() && dir.cd(s)) {
                m_presets.append("");
                for (auto &s : dir.entryList(QDir::Files | QDir::Readable)) {
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

/*!
    \qmlmethod int Filter::savePreset(list<string> propertyNames, string name = "")
    \brief Saves the current values of \a propertyNames as a preset named \a name.
    Returns the index of the new preset in \l presets.
*/

int QmlFilter::savePreset(const QStringList &propertyNames, const QString &name)
{
    Mlt::Properties properties;
    QDir dir(Settings.appDataLocation());

    properties.pass_list(m_service, propertyNames.join('\t').toLatin1().constData());

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
    QString preset = name.isEmpty() ? tr("(defaults)")
                                    : QString::fromUtf8(QUrl::toPercentEncoding(name));
    // Convert properties to YAML string.
    char *yamlStr = properties.serialise_yaml();
    QString yaml = yamlStr;
    free(yamlStr);
    // Save YAML to file
    QFile yamlFile(dir.filePath(preset));
    if (!yamlFile.open(QIODevice::WriteOnly)) {
        LOG_ERROR() << "Failed to save preset: " << dir.filePath(preset);
    }
    yamlFile.write(yaml.toUtf8());
    yamlFile.close();
    loadPresets();
    return m_presets.indexOf(name);
}

/*!
    \qmlmethod Filter::deletePreset(string name)
    \brief Deletes the saved preset named \a name.
*/

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

/*!
    \qmlmethod Filter::analyze(bool isAudio = false, bool deferJob = true)
    \brief Starts an analysis job for this filter.
    Set \a isAudio to \c true for audio filters. When \a deferJob is \c true
    the job is queued rather than run immediately. Emits \l analyzeFinished when done.
*/

void QmlFilter::analyze(bool isAudio, bool deferJob)
{
    // Analyze is only supported for filters, not links.
    if (m_service.type() != mlt_service_filter_type)
        return;

    Mlt::Filter mltFilter(m_service);
    Mlt::Service service(mlt_service(mltFilter.get_data("service")));
    auto in = service.get_int("in");
    auto out = service.get_int("out");

    // get temp file for input xml
    QString filename(mltFilter.get("filename"));
    QScopedPointer<QTemporaryFile> tmp(Util::writableTemporaryFile(filename));
    if (!tmp->open()) {
        LOG_ERROR() << "Failed to create temporary file" << tmp->fileName();
        return;
    }

    mltFilter.clear("results");
    int disable = mltFilter.get_int("disable");
    mltFilter.set("disable", 0);
    if (!isAudio)
        mltFilter.set("analyze", 1);

    // Tag the filter with a UUID stored in a shotcut property to uniquely find it later
    auto uuid = QUuid::createUuid();
    auto ba = uuid.toByteArray();
    mltFilter.set(kShotcutHashProperty, ba.constData());

    // Fix in/out points of filters on clip-only project.
    if (MLT.isSeekableClip() && mlt_service_chain_type != MLT.producer()->type()) {
        Mlt::Producer producer(MLT.profile(), "xml-string", MLT.XML().toUtf8().constData());
        service = Mlt::Service(producer);
        int producerIn = producer.get_in();
        if (producerIn > 0) {
            int n = producer.filter_count();
            for (int i = 0; i < n; i++) {
                Mlt::Filter filter(*producer.filter(i));
                if (filter.get_in() > 0)
                    filter.set_in_and_out(filter.get_in() - producerIn,
                                          filter.get_out() - producerIn);
            }
        }
    } else {
        service.set("in", mltFilter.get_in());
        service.set("out", mltFilter.get_out());
        if (mlt_service_producer_type == service.type())
            mltFilter.set_in_and_out(0, mltFilter.get_length() - 1);
    }

    // Write the job XML
    MLT.saveXML(tmp->fileName(), &service, false /* without relative paths */, tmp.data());
    tmp->close();

    if (!MLT.isSeekableClip()) {
        service.set("in", in);
        service.set("out", out);
    }

    if (!isAudio)
        mltFilter.set("analyze", 0);
    mltFilter.set("disable", disable);

    // get temp filename for output xml
    QScopedPointer<QTemporaryFile> tmpTarget(Util::writableTemporaryFile(filename));
    if (!tmpTarget->open()) {
        LOG_ERROR() << "Failed to create temporary file" << tmpTarget->fileName();
        return;
    }
    tmpTarget->close();

    // parse xml
    QFile f1(tmp->fileName());
    if (!f1.open(QIODevice::ReadOnly)) {
        LOG_ERROR() << "Failed to open temporary file for reading" << tmp->fileName();
        return;
    }
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
    consumerNode.setAttribute("resource", tmpTarget->fileName());

    AbstractJob *job = new MeltJob(tmpTarget->fileName(),
                                   dom.toString(2),
                                   MLT.profile().frame_rate_num(),
                                   MLT.profile().frame_rate_den());
    if (job) {
        AnalyzeDelegate *delegate = new AnalyzeDelegate(mltFilter);
        connect(job, &AbstractJob::finished, delegate, &AnalyzeDelegate::onAnalyzeFinished);
        connect(job, &AbstractJob::finished, this, &QmlFilter::analyzeFinished);
        job->setLabel(tr("Analyze %1").arg(Util::baseName(ProxyManager::resource(service))));

        // Touch the target .stab file. This prevents multiple jobs from trying
        // to write the same file.
        if (!filename.isEmpty() && !QFile::exists(filename)) {
            job->setProperty("filename", filename);
            QFile file(filename);
            if (file.open(QFile::WriteOnly)) {
                file.write("");
            }
        }
        if (deferJob) {
            QTimer::singleShot(0, this, [=]() { JOBS.add(job); });
        } else {
            JOBS.add(job);
        }
    }
}

/*!
    \qmlmethod int Filter::framesFromTime(string time)
    \brief Converts a timecode \a time string (HH:MM:SS.ms or MLT time format) to a frame number.
*/

int QmlFilter::framesFromTime(const QString &time)
{
    if (MLT.producer()) {
        return MLT.producer()->time_to_frames(time.toLatin1().constData());
    }
    return 0;
}

/*!
    \qmlmethod Filter::getHash()
    \brief Computes a hash of the filter's input producer and stores it as the
    \c shotcut:hash property. Used internally by analysis jobs.
*/

void QmlFilter::getHash()
{
    if (m_service.is_valid())
        Util::getHash(m_service);
}

/*!
    \qmlproperty int Filter::in
    \brief The filter's in-point in frames, relative to the clip start.
*/

int QmlFilter::in()
{
    int result = 0;
    if (m_service.is_valid()) {
        if (m_service.type() == mlt_service_link_type
            || (m_service.get_int("in") == 0
                && m_service.get_int("out") == 0)) { // undefined/always-on
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
            result = m_service.get_int("in");
        }
    }
    return result;
}

/*!
    \qmlproperty int Filter::out
    \brief The filter's out-point in frames, relative to the clip start.
*/

int QmlFilter::out()
{
    int result = 0;
    if (m_service.is_valid()) {
        if (m_service.type() == mlt_service_link_type
            || (m_service.get_int("in") == 0
                && m_service.get_int("out") == 0)) { // undefined/always-on
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
            result = m_service.get_int("out");
        }
    }
    return result;
}

/*!
    \qmlproperty int Filter::animateIn
    \brief Number of frames over which the "animate in" simple keyframe ramp runs.
    Set to 0 to disable. Writing this triggers the \l animateInChanged signal.
*/

int QmlFilter::animateIn()
{
    return m_service.time_to_frames(m_service.get(kShotcutAnimInProperty));
}

void QmlFilter::setAnimateIn(int value)
{
    value = qBound(0, value, duration());
    if (value != m_service.time_to_frames(m_service.get(kShotcutAnimInProperty))) {
        m_service.set(kShotcutAnimInProperty, m_service.frames_to_time(value, mlt_time_clock));
        if (value == 0 && m_service.time_to_frames(m_service.get(kShotcutAnimOutProperty)) == 0) {
            // Clear simple keyframes
            for (int i = 0; i < m_metadata->keyframes()->parameterCount(); i++) {
                QString name = m_metadata->keyframes()->parameter(i)->property();
                Mlt::Animation anim = getAnimation(name);
                if (anim.is_valid() && anim.key_count() > 0) {
                    QString value;
                    if (anim.key_count() > 1) {
                        // The second keyframe should be the "middle" simple keyframe.
                        value = m_service.anim_get(qUtf8Printable(name), 1);
                    } else {
                        // Failsafe. This should not happen
                        value = m_service.anim_get(qUtf8Printable(name), 0);
                    }
                    m_service.clear(qUtf8Printable(name));
                    m_service.set(qUtf8Printable(name), qUtf8Printable(value));
                }
            }
        }
        updateUndoCommand(kShotcutAnimInProperty);
        emit animateInChanged();
    }
}

/*!
    \qmlproperty int Filter::animateOut
    \brief Number of frames over which the "animate out" simple keyframe ramp runs.
    Set to 0 to disable. Writing this triggers the \l animateOutChanged signal.
*/

int QmlFilter::animateOut()
{
    return m_service.time_to_frames(m_service.get(kShotcutAnimOutProperty));
}

void QmlFilter::setAnimateOut(int value)
{
    value = qBound(0, value, duration());
    if (value != m_service.time_to_frames(m_service.get(kShotcutAnimOutProperty))) {
        m_service.set(kShotcutAnimOutProperty, m_service.frames_to_time(value, mlt_time_clock));
        if (value == 0 && m_service.time_to_frames(m_service.get(kShotcutAnimInProperty)) == 0) {
            // Clear simple keyframes
            for (int i = 0; i < m_metadata->keyframes()->parameterCount(); i++) {
                QString name = m_metadata->keyframes()->parameter(i)->property();
                Mlt::Animation anim = getAnimation(name);
                if (anim.is_valid() && anim.key_count() > 0) {
                    QString value;
                    // The first keyframe value should be the "middle" simple keyframe
                    value = m_service.anim_get(qUtf8Printable(name), 0);
                    m_service.clear(qUtf8Printable(name));
                    m_service.set(qUtf8Printable(name), qUtf8Printable(value));
                }
            }
        }
        updateUndoCommand(kShotcutAnimOutProperty);
        emit animateOutChanged();
    }
}

void QmlFilter::clearAnimateInOut()
{
    bool inChanged = false;
    bool outChanged = false;
    if (0 != m_service.time_to_frames(m_service.get(kShotcutAnimInProperty))) {
        m_service.set(kShotcutAnimInProperty, m_service.frames_to_time(0, mlt_time_clock));
        inChanged = true;
    }
    if (0 != m_service.time_to_frames(m_service.get(kShotcutAnimOutProperty))) {
        m_service.set(kShotcutAnimOutProperty, m_service.frames_to_time(0, mlt_time_clock));
        outChanged = true;
    }
    if (inChanged)
        emit animateInChanged();
    if (outChanged)
        emit animateOutChanged();
}

/*!
    \qmlproperty int Filter::duration
    \brief Total duration of the filter in frames (\c out - \c in + 1).
*/

int QmlFilter::duration()
{
    return out() - in() + 1;
}

Mlt::Animation QmlFilter::getAnimation(const QString &name)
{
    if (m_service.is_valid()) {
        if (!m_service.get_animation(qUtf8Printable(name))) {
            // Cause a string property to be interpreted as animated value.
            m_service.anim_get_double(qUtf8Printable(name), 0, duration());
        }
        return m_service.get_animation(qUtf8Printable(name));
    }
    return Mlt::Animation();
}

/*!
    \qmlmethod int Filter::keyframeCount(string name)
    \brief Returns the number of keyframes set on property \a name.
*/

int QmlFilter::keyframeCount(const QString &name)
{
    return getAnimation(name).key_count();
}

/*!
    \qmlmethod Filter::resetProperty(string name)
    \brief Removes all keyframes for property \a name and resets it to its default value.
*/

void QmlFilter::resetProperty(const QString &name)
{
    m_service.clear(qUtf8Printable(name));
    emit changed(name.toUtf8().constData());
}

/*!
    \qmlmethod Filter::clearSimpleAnimation(string name)
    \brief Clears the "animate in/out" simple keyframe mode for property \a name,
    leaving only the static value at the current time position.
*/

void QmlFilter::clearSimpleAnimation(const QString &name)
{
    // Reset the animation if there are no keyframes yet.
    if (animateIn() <= 0 && animateOut() <= 0 && keyframeCount(name) <= 0)
        resetProperty(name);
    setAnimateIn(0);
    setAnimateOut(0);
}

void QmlFilter::preset(const QString &name)
{
    if (!m_service.is_valid())
        return;
    QDir dir(Settings.appDataLocation());

    if (!dir.cd("presets") || !dir.cd(objectNameOrService()))
        return;

    auto fileName = dir.filePath(QUrl::toPercentEncoding(name));

    // Detect the preset file format
    bool isYaml = false;
    QFile presetFile(fileName);
    if (presetFile.open(QIODevice::ReadOnly)) {
        if (presetFile.readLine(4) == "---") {
            isYaml = true;
        }
        presetFile.close();
    } else {
        presetFile.setFileName(dir.filePath(name));
        if (presetFile.open(QIODevice::ReadOnly)) {
            fileName = dir.filePath(name);
            if (presetFile.readLine(4) == "---") {
                isYaml = true;
            }
            presetFile.close();
        }
    }

    if (isYaml) {
        // Load from YAML file.
        QScopedPointer<Mlt::Properties> properties(
            Mlt::Properties::parse_yaml(qUtf8Printable(fileName)));
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
            m_service.inherit(*properties);
        }
    } else {
        // Load from legacy preset file
        m_service.load(qUtf8Printable(fileName));
    }

    emit changed();
}

QString QmlFilter::objectNameOrService()
{
    return m_metadata->objectName().isEmpty() ? m_metadata->mlt_service()
                                              : m_metadata->objectName();
}

int QmlFilter::keyframeIndex(Mlt::Animation &animation, int position)
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

void QmlFilter::startUndoTracking()
{
    m_previousState = Mlt::Properties();
    m_previousState.inherit(m_service);
    if (!m_previousState.property_exists(kShotcutAnimInProperty)) {
        m_previousState.set(kShotcutAnimInProperty, 0);
    }
    if (!m_previousState.property_exists(kShotcutAnimOutProperty)) {
        m_previousState.set(kShotcutAnimOutProperty, 0);
    }
}

void QmlFilter::stopUndoTracking()
{
    if (!m_previousState.count()) {
        LOG_DEBUG() << "Undo tracking has not started yet";
        return;
    }
    m_previousState = Mlt::Properties();
}

/*!
    \qmlmethod Filter::startUndoParameterCommand(string description = "")
    \brief Begins an undo/redo command group for parameter changes.
    All \l set() calls until \l endUndoCommand() are grouped as one undoable step.
    \a description appears in the Edit > Undo menu.
*/

void QmlFilter::startUndoParameterCommand(const QString &desc)
{
    if (!m_previousState.count()) {
        //        LOG_DEBUG() << "Undo tracking has not started yet";
        return;
    }
    m_changeInProgress++;
    if (m_changeInProgress > 1) {
        //        LOG_DEBUG() << "Nested change command" << m_changeInProgress;
        return;
    }
    auto command = new Filter::UndoParameterCommand(m_metadata->name(),
                                                    MAIN.filterController(),
                                                    MAIN.filterController()->currentIndex(),
                                                    m_previousState,
                                                    desc);
    MAIN.undoStack()->push(command);
}

void QmlFilter::startUndoAddKeyframeCommand()
{
    if (!m_previousState.count()) {
        //        LOG_DEBUG() << "Undo tracking has not started yet";
        return;
    }
    m_changeInProgress++;
    if (m_changeInProgress > 1) {
        //        LOG_DEBUG() << "Nested change command" << m_changeInProgress;
        return;
    }
    auto command = new Filter::UndoAddKeyframeCommand(m_metadata->name(),
                                                      MAIN.filterController(),
                                                      MAIN.filterController()->currentIndex(),
                                                      m_previousState);
    MAIN.undoStack()->push(command);
}

void QmlFilter::startUndoRemoveKeyframeCommand()
{
    if (!m_previousState.count()) {
        //        LOG_DEBUG() << "Undo tracking has not started yet";
        return;
    }
    m_changeInProgress++;
    if (m_changeInProgress > 1) {
        //        LOG_DEBUG() << "Nested change command" << m_changeInProgress;
        return;
    }
    auto command = new Filter::UndoRemoveKeyframeCommand(m_metadata->name(),
                                                         MAIN.filterController(),
                                                         MAIN.filterController()->currentIndex(),
                                                         m_previousState);
    MAIN.undoStack()->push(command);
}

void QmlFilter::startUndoModifyKeyframeCommand(int paramIndex, int keyframeIndex)
{
    if (!m_previousState.count()) {
        //        LOG_DEBUG() << "Undo tracking has not started yet";
        return;
    }
    m_changeInProgress++;
    if (m_changeInProgress > 1) {
        //        LOG_DEBUG() << "Nested change command" << m_changeInProgress;
        return;
    }
    auto command = new Filter::UndoModifyKeyframeCommand(m_metadata->name(),
                                                         MAIN.filterController(),
                                                         MAIN.filterController()->currentIndex(),
                                                         m_previousState,
                                                         paramIndex,
                                                         keyframeIndex);
    MAIN.undoStack()->push(command);
}

void QmlFilter::updateUndoCommand(const QString &name)
{
    if (!m_previousState.count()) {
        //        LOG_DEBUG() << "Undo tracking has not started yet";
        return;
    }
    if (!m_changeInProgress) {
        startUndoParameterCommand(QString());
    }

    const QUndoCommand *lastCommand = MAIN.undoStack()->command(MAIN.undoStack()->count() - 1);
    Filter::UndoParameterCommand *command = dynamic_cast<Filter::UndoParameterCommand *>(
        const_cast<QUndoCommand *>(lastCommand));
    if (command) {
        // Update the change that is already in progress
        command->update(name);
    } else {
        LOG_ERROR() << "Unable to find command in progress";
        return;
    }
    m_previousState.pass_property(m_service, name.toUtf8().constData());
}

/*!
    \qmlmethod Filter::endUndoCommand()
    \brief Ends the undo/redo command group started by \l startUndoParameterCommand().
*/

void QmlFilter::endUndoCommand()
{
    if (!m_previousState.count()) {
        //        LOG_DEBUG() << "Undo tracking has not started yet";
        return;
    }
    if (!m_changeInProgress) {
        LOG_ERROR() << "Change is not in progress";
        return;
    }
    m_changeInProgress--;
}

mlt_keyframe_type QmlFilter::getKeyframeType(Mlt::Animation &animation,
                                             int position,
                                             mlt_keyframe_type defaultType)
{
    mlt_keyframe_type result = mlt_keyframe_linear;
    if (animation.is_valid()) {
        mlt_keyframe_type existingType = defaultType;
        if (animation.is_key(position)) {
            existingType = animation.key_get_type(keyframeIndex(animation, position));
        } else if (defaultType < 0) {
            int previous = 0;
            bool error = animation.previous_key(position, previous);
            if (!error)
                existingType = animation.keyframe_type(previous);
        }
        if (existingType >= 0)
            result = existingType;
    }
    return result;
}

/*!
    \qmlmethod int Filter::getKeyFrameType(string name, int keyIndex)
    \brief Returns the interpolation type (\c mlt_keyframe_type) of keyframe \a keyIndex
    for property \a name.
*/

int QmlFilter::getKeyFrameType(const QString &name, int keyIndex)
{
    Mlt::Animation animation = getAnimation(name);
    return (int) animation.key_get_type(keyIndex);
}

/*!
    \qmlmethod Filter::setKeyFrameType(string name, int keyIndex, int type)
    \brief Sets the interpolation \a type of keyframe \a keyIndex for property \a name.
*/

void QmlFilter::setKeyFrameType(const QString &name, int keyIndex, int type)
{
    Mlt::Animation animation = getAnimation(name);
    animation.key_set_type(keyIndex, (mlt_keyframe_type) type);
}

/*!
    \qmlmethod int Filter::getNextKeyframePosition(string name, int position)
    \brief Returns the position of the next keyframe for property \a name
    after time \a position, or \c -1 if none.
*/

int QmlFilter::getNextKeyframePosition(const QString &name, int position)
{
    int result = -1;
    Mlt::Animation animation = getAnimation(name);
    if (animation.is_valid()) {
        animation.next_key(animation.is_key(position) ? position + 1 : position, result);
    }
    return result;
}

/*!
    \qmlmethod int Filter::getPrevKeyframePosition(string name, int position)
    \brief Returns the position of the previous keyframe for property \a name
    before time \a position, or \c -1 if none.
*/

int QmlFilter::getPrevKeyframePosition(const QString &name, int position)
{
    int result = -1;
    Mlt::Animation animation = getAnimation(name);
    if (animation.is_valid()) {
        animation.previous_key(animation.is_key(position) ? position - 1 : position, result);
    }
    return result;
}

/*!
    \qmlmethod bool Filter::isAtLeastVersion(string version)
    \brief Returns \c true if Shotcut's version is greater than or equal to \a version.
    Use to guard features introduced in specific releases.
*/

bool QmlFilter::isAtLeastVersion(const QString &version)
{
    QVersionNumber v1 = QVersionNumber::fromString(version);
    QVersionNumber v2 = QVersionNumber::fromString(m_metadata->property("version").toString());
    return v2 >= v1;
}

/*!
    \qmlmethod Filter::deselect()
    \brief Deselects the current filter in the Filters panel.
*/

void QmlFilter::deselect()
{
    MAIN.filterController()->setCurrentFilter(DeselectCurrentFilter);
}

bool QmlFilter::allowTrim() const
{
    if (m_metadata && m_metadata->keyframes())
        return m_metadata->keyframes()->allowTrim();
    return false;
}

bool QmlFilter::allowAnimateIn() const
{
    if (m_metadata && m_metadata->keyframes())
        return m_metadata->keyframes()->allowAnimateIn();
    return false;
}

bool QmlFilter::allowAnimateOut() const
{
    if (m_metadata && m_metadata->keyframes())
        return m_metadata->keyframes()->allowAnimateOut();
    return false;
}

/*!
    \qmlmethod Filter::copyParameters()
    \brief Copies this filter's current parameters to the clipboard.
*/

void QmlFilter::copyParameters()
{
    auto name = "color";
    Mlt::Producer dummy(MLT.profile(), name);
    dummy.inherit(m_service);
    dummy.set("mlt_service", name);
    QGuiApplication::clipboard()->setText(MLT.XML(&dummy));
}

/*!
    \qmlmethod Filter::pasteParameters(list<string> propertyNames)
    \brief Pastes parameters from the clipboard into this filter,
    restricted to the properties listed in \a propertyNames.
*/

void QmlFilter::pasteParameters(const QStringList &propertyNames)
{
    auto xml = QGuiApplication::clipboard()->text();
    Mlt::Producer producer(MLT.profile(), "xml-string", xml.toUtf8().constData());
    if (!producer.is_valid()) {
        LOG_WARNING() << "failed to parse MLT XML on clipboard" << xml;
        return;
    }
    auto isChanged = false;
    for (const auto &name : propertyNames) {
        if (producer.property_exists(name.toUtf8().constData())) {
            LOG_DEBUG() << name << "=" << producer.get(name.toUtf8().constData());
            m_service.pass_property(producer, name.toUtf8().constData());
            isChanged = true;
            emit changed(name);
        }
    }
    if (isChanged)
        emit changed();
}

/*!
    \qmlmethod Filter::crop(rect r)
    \brief Applies a crop rectangle \a r to the filter's producer source.
*/

void QmlFilter::crop(const QRectF &rect)
{
    MAIN.cropSource(rect);
}

AnalyzeDelegate::AnalyzeDelegate(Mlt::Filter &filter)
    : QObject(nullptr)
    , m_uuid(filter.get(kShotcutHashProperty))
{}

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

    QList<Mlt::Filter> &filters() { return m_filters; }

    int on_start_filter(Mlt::Filter *filter)
    {
        QByteArray uuid = filter->get(kShotcutHashProperty);
        if (uuid == m_uuid.toByteArray())
            m_filters << Mlt::Filter(*filter);
        return 0;
    }
    int on_start_producer(Mlt::Producer *) { return 0; }
    int on_end_producer(Mlt::Producer *) { return 0; }
    int on_start_playlist(Mlt::Playlist *) { return 0; }
    int on_end_playlist(Mlt::Playlist *) { return 0; }
    int on_start_tractor(Mlt::Tractor *) { return 0; }
    int on_end_tractor(Mlt::Tractor *) { return 0; }
    int on_start_multitrack(Mlt::Multitrack *) { return 0; }
    int on_end_multitrack(Mlt::Multitrack *) { return 0; }
    int on_start_track() { return 0; }
    int on_end_track() { return 0; }
    int on_end_filter(Mlt::Filter *) { return 0; }
    int on_start_transition(Mlt::Transition *) { return 0; }
    int on_end_transition(Mlt::Transition *) { return 0; }
    int on_start_chain(Mlt::Chain *) { return 0; }
    int on_end_chain(Mlt::Chain *) { return 0; }
    int on_start_link(Mlt::Link *) { return 0; }
    int on_end_link(Mlt::Link *) { return 0; }
};

void AnalyzeDelegate::updateJob(EncodeJob *job, const QString &results)
{
    bool isUpdated = false;

    // parse the xml
    QFile file(job->xmlPath());
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR() << "Failed to open job XML for reading" << job->xmlPath();
        return;
    }
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
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream textStream(&file);
            dom.save(textStream, 2);
            file.close();
        } else {
            LOG_ERROR() << "Failed to open job XML for writing" << job->xmlPath();
        }
    }
}

void AnalyzeDelegate::onAnalyzeFinished(AbstractJob *job, bool isSuccess)
{
    QString fileName = job->objectName();

    if (isSuccess) {
        QString results = resultsFromXml(fileName);
        if (!results.isEmpty()) {
            // look for filters by UUID in each pending export job.
            foreach (AbstractJob *job, JOBS.jobs()) {
                if (!job->ran() && typeid(*job) == typeid(EncodeJob)) {
                    updateJob(dynamic_cast<EncodeJob *>(job), results);
                }
            }

            // Locate filters in memory by UUID.
            if (MAIN.isMultitrackValid()) {
                FindFilterParser graphParser(m_uuid);
                graphParser.start(*MAIN.multitrack());
                foreach (Mlt::Filter filter, graphParser.filters())
                    updateFilter(filter, results);
            }
            if (MAIN.playlist() && MAIN.playlist()->count() > 0) {
                FindFilterParser graphParser(m_uuid);
                graphParser.start(*MAIN.playlist());
                foreach (Mlt::Filter filter, graphParser.filters())
                    updateFilter(filter, results);
            }
            Mlt::Producer producer(MLT.isClip() ? MLT.producer() : MLT.savedProducer());
            if (producer.is_valid()) {
                FindFilterParser graphParser(m_uuid);
                graphParser.start(producer);
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

QString AnalyzeDelegate::resultsFromXml(const QString &fileName)
{
    // parse the xml
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR() << "Failed to open job XML for reading" << fileName;
        return QString();
    }
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
            if (propertyNode.attributes().namedItem("name").toAttr().value() == kShotcutHashProperty
                && propertyNode.toElement().text() == m_uuid.toString()) {
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

void AnalyzeDelegate::updateFilter(Mlt::Filter &filter, const QString &results)
{
    filter.set("results", qUtf8Printable(results));
    filter.set("reload", 1);
    filter.clear(kShotcutHashProperty);
    LOG_INFO() << "updated filter" << filter.get("mlt_service") << "with results:" << results;

    if (QString::fromLatin1("opencv.tracker") == filter.get("mlt_service")) {
        auto model = MAIN.filterController()->motionTrackerModel();
        if (model) {
            auto name = QString::fromUtf8(filter.get(kTrackNameProperty));
            if (name.isEmpty()) {
                name = model->nextName();
                filter.set(kTrackNameProperty, name.toUtf8().constData());
            }
            auto key = model->keyForFilter(&filter);
            if (key.isEmpty()) {
                key = model->add(name, results);
                if (!key.isEmpty()) {
                    filter.set(kUuidProperty, key.toUtf8().constData());
                }
            } else {
                model->updateData(key, results);
            }
        }
    }
}
