/*
 * Copyright (c) 2013-2016 Meltytech, LLC
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
#include <QDir>
#include <QIODevice>
#include <QTemporaryFile>
#include <QFile>
#include <QtXml>
#include <MltProducer.h>

static const char* kWidthProperty = "meta.media.width";
static const char* kHeightProperty = "meta.media.height";
static const char* kAspectNumProperty = "meta.media.sample_aspect_num";
static const char* kAspectDenProperty = "meta.media.sample_aspect_den";

QmlFilter::QmlFilter(Mlt::Filter* mltFilter, const QmlMetadata* metadata, QObject* parent)
    : QObject(parent)
    , m_metadata(metadata)
    , m_filter(mltFilter)
    , m_path(m_metadata->path().absolutePath().append('/'))
    , m_isNew(false)
{
}

QmlFilter::~QmlFilter()
{
    delete m_filter;
}

QString QmlFilter::get(QString name)
{
    if (m_filter)
        return QString::fromUtf8(m_filter->get(name.toUtf8().constData()));
    else
        return QString();
}

double QmlFilter::getDouble(QString name)
{
    if (m_filter)
        return m_filter->get_double(name.toUtf8().constData());
    else
        return 0;
}

QRectF QmlFilter::getRect(QString name)
{
    const char* s = m_filter->get(name.toUtf8().constData());
    if (s) {
        mlt_rect rect = m_filter->get_rect(name.toUtf8().constData());
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

void QmlFilter::set(QString name, QString value)
{
    if (!m_filter) return;
    if (qstrcmp(m_filter->get(name.toUtf8().constData()), value.toUtf8().constData())) {
        m_filter->set(name.toUtf8().constData(), value.toUtf8().constData());
        MLT.refreshConsumer();
        emit changed();
    }
}

void QmlFilter::set(QString name, double value)
{
    if (!m_filter) return;
    if (!m_filter->get(name.toUtf8().constData())
        || m_filter->get_double(name.toUtf8().constData()) != value) {
        m_filter->set(name.toUtf8().constData(), value);
        MLT.refreshConsumer();
        emit changed();
    }
}

void QmlFilter::set(QString name, int value)
{
    if (!m_filter) return;
    if (!m_filter->get(name.toUtf8().constData())
        || m_filter->get_int(name.toUtf8().constData()) != value) {
        m_filter->set(name.toUtf8().constData(), value);
        MLT.refreshConsumer();
        emit changed();
    }
}

void QmlFilter::set(QString name, double x, double y, double width, double height, double opacity)
{
    if (!m_filter) return;
    mlt_rect rect = m_filter->get_rect(name.toUtf8().constData());
    if (!m_filter->get(name.toUtf8().constData()) || x != rect.x || y != rect.y
        || width != rect.w || height != rect.h || opacity != rect.o) {
        m_filter->set(name.toUtf8().constData(), x, y, width, height, opacity);
        MLT.refreshConsumer();
        emit changed();
    }
}

void QmlFilter::loadPresets()
{
    m_presets.clear();
    QDir dir(Settings.appDataLocation());
    if (dir.cd("presets")) {
        QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Executable);
        foreach (QString s, entries) {
            if (s == objectNameOrService() && dir.cd(s)) {
                m_presets.append(" ");
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

    properties.pass_list(*((Mlt::Properties*)m_filter), propertyNames.join('\t').toLatin1().constData());

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
    properties.save(dir.filePath(preset).toUtf8().constData());
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
    Mlt::Service service(mlt_service(m_filter->get_data("service")));

    // get temp filename for input xml
    QTemporaryFile tmp;
    tmp.open();
    tmp.close();
    m_filter->set("results", NULL, 0);
    int disable = m_filter->get_int("disable");
    m_filter->set("disable", 0);
    MLT.saveXML(tmp.fileName(), &service);
    m_filter->set("disable", disable);

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

    AbstractJob* job = new MeltJob(tmpTarget.fileName(), dom.toString(2));
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

QString QmlFilter::timeFromFrames(int frames)
{
    if (MLT.producer()) {
        return MLT.producer()->frames_to_time(frames);
    }
    return QString();
}

void QmlFilter::getHash()
{
    MAIN.getHash(*m_filter);
}

int QmlFilter::producerIn() const
{
    // Every attached filter has a service property that points to the service to
    // which it is attached.
    Mlt::Producer producer(mlt_producer(m_filter->get_data("service")));
    if (producer.get(kFilterInProperty))
        // Shots on the timeline will set the producer to the cut parent.
        // However, we want time-based filters such as fade in/out to use
        // the cut's in/out and not the parent's.
        return producer.get_int(kFilterInProperty);
    else
        return producer.get_in();
}

int QmlFilter::producerOut() const
{
    // Every attached filter has a service property that points to the service to
    // which it is attached.
    Mlt::Producer producer(mlt_producer(m_filter->get_data("service")));
    if (producer.get(kFilterOutProperty))
        // Shots on the timeline will set the producer to the cut parent.
        // However, we want time-based filters such as fade in/out to use
        // the cut's in/out and not the parent's.
        return producer.get_int(kFilterOutProperty);
    else
        return producer.get_out();
}

double QmlFilter::producerAspect() const
{
    // Every attached filter has a service property that points to the service to
    // which it is attached.
    Mlt::Producer producer(mlt_producer(m_filter->get_data("service")));
    if (producer.get(kHeightProperty)) {
        double sar = 1.0;
        if (producer.get(kAspectDenProperty)) {
            sar = producer.get_double(kAspectNumProperty) /
                  producer.get_double(kAspectDenProperty);
        }
        return sar * producer.get_double(kWidthProperty) / producer.get_double(kHeightProperty);
    }
    return MLT.profile().dar();
}

void QmlFilter::preset(const QString &name)
{
    QDir dir(Settings.appDataLocation());

    if (!dir.cd("presets") || !dir.cd(objectNameOrService()))
        return;
    m_filter->load(dir.filePath(name).toUtf8().constData());
    MLT.refreshConsumer();
    emit changed();
}

QString QmlFilter::objectNameOrService()
{
    return m_metadata->objectName().isEmpty()? m_metadata->mlt_service() : m_metadata->objectName();
}

AnalyzeDelegate::AnalyzeDelegate(Mlt::Filter* filter)
    : QObject(0)
    , m_filter(*filter)
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
