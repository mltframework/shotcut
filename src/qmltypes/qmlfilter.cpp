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
#include "jobqueue.h"
#include <QStandardPaths>
#include <QDir>
#include <QIODevice>
#include <QTemporaryFile>
#include <QFile>
#include <QtXml>

QmlFilter::QmlFilter(AttachedFiltersModel& model, const QmlMetadata &metadata, int row, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_metadata(metadata)
    , m_path(m_metadata.path().absolutePath().append('/'))
    , m_isNew(row < 0)
{
    if (m_isNew) {
        m_filter = m_model.add(m_metadata.mlt_service(), m_metadata.objectName());
    }
    else {
        m_filter = model.filterForRow(row);
    }
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

void QmlFilter::loadPresets()
{
    m_presets.clear();
    QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
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
    QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());

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
    if (!QFile(dir.filePath(preset)).exists())
        properties.save(dir.filePath(preset).toUtf8().constData());
    loadPresets();
    return m_presets.indexOf(name);
}

void QmlFilter::deletePreset(const QString &name)
{
    QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
    if (dir.cd("presets") && dir.cd(objectNameOrService()))
        QFile(dir.filePath(name)).remove();
    m_presets.removeOne(name);
    emit presetsChanged();
}

void QmlFilter::analyze()
{
    // get temp filename for input xml
    QTemporaryFile tmp(QDir::tempPath().append("/shotcut-XXXXXX"));
    tmp.open();
    QString tmpName = tmp.fileName();
    tmp.close();
    tmpName.append(".mlt");
    m_filter->set("results", NULL, 0);
    int disable = m_filter->get_int("disable");
    m_filter->set("disable", 0);
    MLT.saveXML(tmpName);
    m_filter->set("disable", disable);

    // get temp filename for output xml
    QTemporaryFile tmpTarget(QDir::tempPath().append("/shotcut-XXXXXX"));
    tmpTarget.open();
    QString target = tmpTarget.fileName();
    tmpTarget.close();
    target.append(".mlt");

    // parse xml
    QFile f1(tmpName);
    f1.open(QIODevice::ReadOnly);
    QDomDocument dom(tmpName);
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
    consumerNode.setAttribute("audio_off", 1);
    consumerNode.setAttribute("no_meta", 1);
    consumerNode.setAttribute("resource", target);

    // save new xml
    f1.open(QIODevice::WriteOnly);
    QTextStream ts(&f1);
    dom.save(ts, 2);
    f1.close();

    MeltJob* job = new MeltJob(target, tmpName);
    if (job) {
        connect(job, SIGNAL(finished(MeltJob*, bool)), SLOT(onAnalyzeFinished(MeltJob*, bool)));
        QFileInfo info(MLT.resource());
        job->setLabel(tr("Analyze %1").arg(info.fileName()));
        JOBS.add(job);
    }
}

void QmlFilter::preset(const QString &name)
{
    QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());

    if (!dir.cd("presets") || !dir.cd(objectNameOrService()))
        return;
    m_filter->load(dir.filePath(name).toUtf8().constData());
    MLT.refreshConsumer();
}

QString QmlFilter::objectNameOrService()
{
    return m_metadata.objectName().isEmpty()? m_metadata.mlt_service() : m_metadata.objectName();
}

void QmlFilter::onAnalyzeFinished(MeltJob *job, bool isSuccess)
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
                        && propertyNode.toElement().text() == get("mlt_service")) {
                    found = true;
                    break;
                }
            }
            if (found) {
                for (int j = 0; j < properties.size(); j++) {
                    QDomNode propertyNode = properties.at(j);
                    if (propertyNode.attributes().namedItem("name").toAttr().value() == "results") {
                        m_filter->set("results", propertyNode.toElement().text().toLatin1().constData());
                    }
                }
                break;
            }
        }
    }
    QFile::remove(fileName);
    emit analyzeFinished(isSuccess);
}
