/*
 * Copyright (c) 2014-2023 Meltytech, LLC
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

#include "mltxmlchecker.h"
#include "mltcontroller.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include "proxymanager.h"
#include "settings.h"

#include <QLocale>
#include <QDir>
#include <QCoreApplication>
#include <QUrl>
#include <QRegularExpression>
#include <Logger.h>
#include <clocale>
#include <utime.h>

static QString getPrefix(const QString &name, const QString &value);

static bool isMltClass(const QString &name)
{
    return name == "profile" || name == "producer" ||
           name == "filter" || name == "playlist" ||
           name == "tractor" || name == "track" ||
           name == "transition" || name == "consumer" ||
           name == "chain" || name == "link";
}

static bool isNetworkResource(const QString &string)
{
    // Check if it looks like a qualified URL. Try parsing it and see.
    QRegularExpression schemaTest(QLatin1String("^[a-zA-Z]{2,}\\:.*"));
    // Not actually checking network URL due to latency and transience.
    return (schemaTest.match(string).hasMatch() && QUrl(string).isValid()
            && !string.startsWith("plain:"));
}

static bool isNumericProperty(const QString &name)
{
    return  name == "length" || name == "geometry" ||
            name == "rect" || name == "warp_speed";
}

MltXmlChecker::MltXmlChecker()
    : m_needsGPU(false)
    , m_needsCPU(false)
    , m_hasEffects(false)
    , m_isCorrected(false)
    , m_isUpdated(false)
    , m_decimalPoint('.')
    , m_numericValueChanged(false)
{
    m_unlinkedFilesModel.setColumnCount(ColumnCount);
}

QXmlStreamReader::Error MltXmlChecker::check(const QString &fileName)
{
    LOG_DEBUG() << "begin";

    QFile file(fileName);
    m_tempFile.reset(new QTemporaryFile(
                         QFileInfo(fileName).dir().filePath("shotcut-XXXXXX.mlt")));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) && m_tempFile->open()) {
        m_tempFile->resize(0);
        m_fileInfo = QFileInfo(fileName);
        m_xml.setDevice(&file);
        m_newXml.setDevice(m_tempFile.data());
        m_newXml.setAutoFormatting(true);
        m_newXml.setAutoFormattingIndent(2);
        if (m_xml.readNextStartElement()) {
            if (m_xml.name().toString() == "mlt") {
                m_newXml.writeStartDocument();
                m_newXml.writeCharacters("\n");
                m_newXml.writeStartElement("mlt");
                foreach (QXmlStreamAttribute a, m_xml.attributes()) {
                    if (a.name().toString().toUpper() == "LC_NUMERIC") {
                        QString value = a.value().toString().toUpper();
                        m_newXml.writeAttribute("LC_NUMERIC", "C");
                        MLT.resetLocale();
                        m_decimalPoint = '.';
                    } else if (a.name().toString().toLower() == "version") {
                        m_mltVersion = QVersionNumber::fromString(a.value());
                    } else if (a.name().toString().toLower() == "title") {
                        m_newXml.writeAttribute(a.name().toString(), "Shotcut version " SHOTCUT_VERSION);
                        auto parts = a.value().split(' ');
                        LOG_DEBUG() << parts;
                        if (parts.size() > 2 && parts[1].toString() == "version") {
                            m_shotcutVersion = parts[2].toString();
                        }
                    } else {
                        m_newXml.writeAttribute(a);
                    }
                }
                if (!checkMltVersion()) {
                    return QXmlStreamReader::CustomError;
                }

                readMlt();
                m_newXml.writeEndElement();
                m_newXml.writeEndDocument();
                m_isCorrected = m_isCorrected || m_numericValueChanged;
            } else {
                m_xml.raiseError(QObject::tr("The file is not a MLT XML file."));
            }
        }
    }
    LOG_DEBUG() << m_tempFile->fileName();
    if (m_tempFile->isOpen()) {
        m_tempFile->close();

        // Useful for debugging
//        m_tempFile->open();
//        LOG_DEBUG() << m_tempFile->readAll().constData();
//        m_tempFile->close();
    }
    LOG_DEBUG() << "end" << m_xml.errorString();
    return m_xml.error();
}

QString MltXmlChecker::errorString() const
{
    return m_xml.errorString();
}

void MltXmlChecker::readMlt()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().toString() == "mlt");
    bool isPropertyElement = false;

    while (!m_xml.atEnd()) {
        switch (m_xml.readNext()) {
        case QXmlStreamReader::Characters:
            if (!isPropertyElement)
                m_newXml.writeCharacters(m_xml.text().toString());
            break;
        case QXmlStreamReader::Comment:
            m_newXml.writeComment(m_xml.text().toString());
            break;
        case QXmlStreamReader::DTD:
            m_newXml.writeDTD(m_xml.text().toString());
            break;
        case QXmlStreamReader::EntityReference:
            m_newXml.writeEntityReference(m_xml.name().toString());
            break;
        case QXmlStreamReader::ProcessingInstruction:
            m_newXml.writeProcessingInstruction(m_xml.processingInstructionTarget().toString(),
                                                m_xml.processingInstructionData().toString());
            break;
        case QXmlStreamReader::StartDocument:
            m_newXml.writeStartDocument(m_xml.documentVersion().toString(), m_xml.isStandaloneDocument());
            break;
        case QXmlStreamReader::EndDocument:
            m_newXml.writeEndDocument();
            break;
        case QXmlStreamReader::StartElement: {
            const QString element = m_xml.name().toString();
            isPropertyElement = false;
            if (element == "property") {
                isPropertyElement = true;
                if (isMltClass(mlt_class)) {
                    const QString name = m_xml.attributes().value("name").toString();
                    m_properties << MltProperty(name, m_xml.readElementText());
                }
            } else {
                processProperties();
                m_newXml.writeStartElement(m_xml.namespaceUri().toString(), element);
                if (isMltClass(m_xml.name().toString()))
                    mlt_class = element;
                checkInAndOutPoints(); // This also copies the attributes.
            }
            break;
        }
        case QXmlStreamReader::EndElement:
            if (m_xml.name().toString() != "property") {
                processProperties();
                m_newXml.writeEndElement();
                if (isMltClass(m_xml.name().toString())) {
                    mlt_class.clear();
                }
            }
            break;
        default:
            break;
        }
    }
}

void MltXmlChecker::processProperties()
{
    QString mlt_service;
    QVector<MltProperty> newProperties;
    m_resource.clear();

    // First pass: collect information about mlt_service and resource.
    foreach (MltProperty p, m_properties) {
        // Get the name of the MLT service.
        if (p.first == "mlt_service") {
            mlt_service = p.second;
        } else if (p.first == kShotcutHashProperty) {
            m_resource.hash = p.second;
        } else if (p.first.startsWith(kIsProxyProperty)) {
            m_resource.isProxy = true;
        } else if (isNumericProperty(p.first)) {
            checkNumericString(p.second);
        } else if (p.first == "resource" && mlt_service == "webvfx" && fixWebVfxPath(p.second)) {
        } else if (readResourceProperty(p.first, p.second)) {
#ifdef Q_OS_WIN
            fixVersion1701WindowsPathBug(p.second);
#endif
            // Check timewarp producer's resource property prefix.
            QString prefix = getPrefix(p.first, p.second);
            if (!prefix.isEmpty() && prefix != "plain:") {
                if (checkNumericString(prefix))
                    p.second = prefix + p.second.mid(p.second.indexOf(':') + 1);
            }
            fixUnlinkedFile(p.second);
        }
        newProperties << MltProperty(p.first, p.second);
    }

    if (mlt_class == "filter" || mlt_class == "transition" || mlt_class == "producer"
            || mlt_class == "chain" || mlt_class == "link") {
        checkGpuEffects(mlt_service);
        checkCpuEffects(mlt_service);
        checkUnlinkedFile(mlt_service);
        checkIncludesSelf(newProperties);
        checkLumaAlphaOver(mlt_service, newProperties);
#if LIBMLT_VERSION_INT >= ((6<<16)+(23<<8))
        replaceWebVfxCropFilters(mlt_service, newProperties);
        replaceWebVfxChoppyFilter(mlt_service, newProperties);
#endif
        bool proxyEnabled = Settings.proxyEnabled();
        if (proxyEnabled)
            checkForProxy(mlt_service, newProperties);

        // Second pass: amend property values.
        bool relinkMismatch = !m_resource.hash.isEmpty() && !m_resource.newHash.isEmpty()
                              && m_resource.hash != m_resource.newHash;
        m_properties = newProperties;
        newProperties.clear();
        foreach (MltProperty p, m_properties) {
            // Fix some properties if re-linked file.
            if (p.first == kShotcutHashProperty) {
                if (!m_resource.newHash.isEmpty())
                    p.second = m_resource.newHash;
            } else if (p.first == kShotcutCaptionProperty) {
                if (!m_resource.newDetail.isEmpty())
                    p.second = Util::baseName(m_resource.newDetail);
            } else if (p.first == kShotcutDetailProperty) {
                // We no longer save this (leaks absolute paths).
                p.second.clear();
            } else if (relinkMismatch && p.first == "audio_index") {
                // Reset stream index properties if re-linked file is different.
                p.second = QString::number(m_resource.audio_index);
            } else if (relinkMismatch && p.first == "astream") {
                p.second = "0";
            } else if (relinkMismatch && p.first == "video_index") {
                p.second = QString::number(m_resource.video_index);
            } else if (relinkMismatch && p.first == "vstream") {
                p.second = "0";
            } else if (relinkMismatch && p.first.startsWith("meta.")) {
                // Remove meta properties if re-linked file is different.
                p.second.clear();
            } else if ((proxyEnabled
                        && m_resource.notProxyMeta
                        && p.first.startsWith("meta.")) ||
                       (!proxyEnabled
                        && m_resource.isProxy
                        && (p.first == kIsProxyProperty || p.first.startsWith("meta.")))) {
                // Remove meta properties if they misrepresent proxy/original
                p.second.clear();
                m_isUpdated = true;
            }

            if (!p.second.isEmpty())
                newProperties << MltProperty(p.first, p.second);
        }
    }

    // Write all of the properties.
    foreach (MltProperty p, newProperties) {
        m_newXml.writeStartElement("property");
        m_newXml.writeAttribute("name", p.first);
        m_newXml.writeCharacters(p.second);
        m_newXml.writeEndElement();
    }
    m_properties.clear();
}

void MltXmlChecker::checkInAndOutPoints()
{
    Q_ASSERT(m_xml.isStartElement());

    // Fix numeric values of in and out point attributes.
    foreach (QXmlStreamAttribute a, m_xml.attributes()) {
        if (a.name().toString() == "in" || a.name().toString() == "out") {
            QString value = a.value().toString();
            if (checkNumericString(value)) {
                m_newXml.writeAttribute(a.name().toString(), value);
                continue;
            }
        }
        m_newXml.writeAttribute(a);
    }
}

bool MltXmlChecker::checkNumericString(QString &value)
{
    // Determine if there is a decimal point inconsistent with locale.
    if (!value.contains(m_decimalPoint) &&
            (value.contains('.') || value.contains(','))) {
        value.replace(',', m_decimalPoint);
        value.replace('.', m_decimalPoint);
        m_numericValueChanged = true;
        return true;
    }
    return false;
}

bool MltXmlChecker::fixWebVfxPath(QString &resource)
{
    // The path, if absolute, should start with the Shotcut executable path.
    QFileInfo fi(resource);
    if (fi.isAbsolute() || Util::hasDriveLetter(resource)) {
        QDir appPath(QCoreApplication::applicationDirPath());

#if defined(Q_OS_MAC)
        // Leave the MacOS directory
        appPath.cdUp();
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        // Leave the bin directory on Linux.
        appPath.cdUp();
#endif
        if (!resource.startsWith(appPath.path())) {
            // Locate "share/shotcut" and replace the front of it with appPath.
            int i = resource.indexOf("/share/shotcut/");
#if defined(Q_OS_MAC)
            if (i == -1)
                i = resource.indexOf("/Resources/shotcut/");
#endif
            if (i >= 0) {
                resource.replace(0, i, appPath.path());
                m_isUpdated = true;
                return true;
            }
        }
    }
    return false;
}

static QString getPrefix(const QString &name, const QString &value)
{
    int length = 0;

    // Webvfx and timewarp are only using "resource".
    if (name == "resource") {
        const QString plain("plain:");
        // webvfx "plain:"
        if (value.startsWith(plain)) {
            return plain;
        } else {
            // timewarp speed parameter
            length = value.indexOf(':');
            if (length > 0) {
                bool isNumeric = true;
                for (int i = 0; i < length && isNumeric; i++) {
                    if (!value[i].isDigit() && value[i] != '.' && value[i] != ',')
                        isNumeric = false;
                }
                if (isNumeric)
                    return value.left(length + 1); // include the colon
            }
        }
    }
    return QString();
}

static QString getSuffix(const QString &name, const QString &value)
{
    // avformat is only using "resource"
    if (name == "resource") {
        const QString queryDelimiter("\\?");
        const auto i = value.lastIndexOf(queryDelimiter);
        // webvfx "plain:"
        if (i > 0) {
            return value.mid(i);
        }
    }
    return QString();
}

bool MltXmlChecker::readResourceProperty(const QString &name, QString &value)
{
    if (mlt_class == "filter" || mlt_class == "transition" || mlt_class == "producer"
            || mlt_class == "chain" || mlt_class == "link")
        if (name == "resource" || name == "src" || name == "filename"
                || name == "luma" || name == "luma.resource" || name == "composite.luma"
                || name == "producer.resource" || name == "av.file" || name == "warp_resource") {

            // Handle special prefix such as "plain:" or speed.
            m_resource.prefix = getPrefix(name, value);
            m_resource.suffix = getSuffix(name, value);
            auto filePath = value.mid(m_resource.prefix.size());
            filePath = filePath.left(filePath.size() - m_resource.suffix.size());
            // Save the resource name (minus prefix) for later check for unlinked files.
            m_resource.info.setFile(filePath);
            if (!isNetworkResource(value) && m_resource.info.isRelative() && !Util::hasDriveLetter(value))
                m_resource.info.setFile(m_fileInfo.canonicalPath(), m_resource.info.filePath());
            return true;
        }
    return false;
}

void MltXmlChecker::checkGpuEffects(const QString &mlt_service)
{
    // Check for GPU effects.
    if (!MLT.isAudioFilter(mlt_service))
        m_hasEffects = true;
    if (mlt_service.startsWith("movit.") || mlt_service.startsWith("glsl."))
        m_needsGPU = true;
}

void MltXmlChecker::checkCpuEffects(const QString &mlt_service)
{
    if (mlt_service.startsWith("frei0r.cairoblend") || mlt_service.startsWith("choppy"))
        m_needsCPU = true;
}

void MltXmlChecker::checkUnlinkedFile(const QString &mlt_service)
{
    // Check for an unlinked file.
    const QString fileName = m_resource.info.fileName();
    const QString filePath = QDir::toNativeSeparators(m_resource.info.filePath());
    // not the color producer
    if (!mlt_service.isEmpty() && mlt_service != "color" && mlt_service != "colour")
        // not a builtin luma wipe file
        if ((mlt_service != "luma" && mlt_service != "movit.luma_mix") || !fileName.startsWith('%'))
            // not a Stabilize filter without Analyze results
            if (fileName != "vidstab.trf")
                // not the generic <producer> resource
                if (fileName != "<producer>")
                    // not an invalid <tractor>
                    if (fileName != "<tractor>")
                        // not an invalid blank
                        if (mlt_service != "blank" || fileName != "blank")
                            // not a URL
                            if (!m_resource.info.filePath().isEmpty() && !isNetworkResource(m_resource.info.filePath()))
                                // not an image sequence
                                if ((mlt_service != "pixbuf" && mlt_service != "qimage") || fileName.indexOf('%') == -1)
                                    // file does not exist
                                    if (!m_resource.info.exists())
                                        // not already in the model
                                        if (m_unlinkedFilesModel.findItems(filePath,
                                                                           Qt::MatchFixedString | Qt::MatchCaseSensitive).isEmpty()) {
                                            LOG_ERROR() << "file not found: " << m_resource.info.filePath();
                                            QIcon icon(":/icons/oxygen/32x32/status/task-reject.png");
                                            QStandardItem *item = new QStandardItem(icon, filePath);
                                            item->setToolTip(item->text());
                                            item->setData(m_resource.hash, ShotcutHashRole);
                                            m_unlinkedFilesModel.appendRow(item);
                                        }
}

bool MltXmlChecker::fixUnlinkedFile(QString &value)
{
    // Replace unlinked files if model is populated with replacements.
    for (int row = 0; row < m_unlinkedFilesModel.rowCount(); ++row) {
        const QStandardItem *replacement = m_unlinkedFilesModel.item(row, ReplacementColumn);
        if (replacement && !replacement->text().isEmpty() &&
                m_unlinkedFilesModel.item(row,
                                          MissingColumn)->text() == QDir::toNativeSeparators(m_resource.info.filePath())) {
            m_resource.info.setFile(replacement->text());
            m_resource.newDetail = replacement->text();
            m_resource.newHash = replacement->data(ShotcutHashRole).toString();
            value = QDir::fromNativeSeparators(replacement->text());
            // Convert to relative path if possible.
            if (value.startsWith(m_fileInfo.canonicalPath() + "/"))
                value = value.mid(m_fileInfo.canonicalPath().size() + 1);
            // Restore special prefix such as "plain:" or speed value.
            value.prepend(m_resource.prefix);
            value.append(m_resource.suffix);
            m_isCorrected = true;

            Mlt::Producer producer(MLT.profile(), m_resource.info.filePath().toUtf8().constData());
            if (producer.is_valid() && !::qstrcmp(producer.get("mlt_service"), "avformat")) {
                m_resource.audio_index = producer.get_int("audio_index");
                m_resource.video_index = producer.get_int("video_index");
            }
            return true;
        }
    }
    return false;
}

bool MltXmlChecker::fixVersion1701WindowsPathBug(QString &value)
{
    if (value.size() >= 3 && value[0] == '/' && value[2] == ':') {
        value.remove(0, 1);
        m_isCorrected = true;
        return true;
    }
    return false;
}

void MltXmlChecker::checkIncludesSelf(QVector<MltProperty> &properties)
{
    if (m_resource.info.canonicalFilePath() == m_fileInfo.canonicalFilePath()) {
        LOG_WARNING() << "This project tries to include itself; breaking that!";
        for (auto &p : properties) {
            if (p.first == "mlt_service")
                p.second.clear();
            else if (p.first == "resource")
                p.second = "+INVALID.txt";
        }
        properties << MltProperty(kShotcutCaptionProperty, "INVALID");
        m_isCorrected = true;
    }
}

void MltXmlChecker::checkLumaAlphaOver(const QString &mlt_service,
                                       QVector<MltXmlChecker::MltProperty> &properties)
{
    if (mlt_service == "luma") {
        bool found = false;
        for (auto &p : properties) {
            if (p.first == "alpha_over") {
                found = true;
            }
        }
        if (!found) {
            properties << MltProperty("alpha_over", "1");
            m_isUpdated = true;
        }
    }
}

void MltXmlChecker::replaceWebVfxCropFilters(QString &mlt_service,
                                             QVector<MltXmlChecker::MltProperty> &properties)
{
    if (mlt_service == "webvfx") {
        auto isCrop = false;
        for (auto &p : properties) {
            if (p.first == "shotcut:filter" && p.second == "webvfxCircularFrame") {
                p.second = "cropCircle";
                properties << MltProperty("circle", "1");
                m_isUpdated = isCrop = true;
                break;
            }
            if (p.first == "shotcut:filter" && p.second == "webvfxClip") {
                p.second = "cropRectangle";
                m_isUpdated = isCrop = true;
                break;
            }
        }
        if (isCrop) {
            mlt_service = "qtcrop";
            for (auto &p : properties) {
                if (p.first == "resource") {
                    properties.removeOne(p);
                    break;
                }
            }
            for (auto &p : properties) {
                if (p.first == "mlt_service") {
                    p.second = "qtcrop";
                    break;
                }
            }
        }
    }
}

void MltXmlChecker::replaceWebVfxChoppyFilter(QString &mlt_service,
                                              QVector<MltXmlChecker::MltProperty> &properties)
{
    if (mlt_service == "webvfx") {
        auto isChoppy = false;
        QString shotcutFilter;
        for (auto &p : properties) {
            if (p.first == "shotcut:filter") {
                shotcutFilter = p.second;
                if (p.second == "webvfxChoppy") {
                    properties.removeOne(p);
                    m_isUpdated = isChoppy = true;
                    break;
                }
            }
        }
        if (isChoppy) {
            mlt_service = "choppy";
            for (auto &p : properties) {
                if (p.first == "resource") {
                    properties.removeOne(p);
                    break;
                }
            }
            for (auto &p : properties) {
                if (p.first == "mlt_service") {
                    p.second = "choppy";
                    break;
                }
            }
        } else if (shotcutFilter.isEmpty()) {
            mlt_service = "qtext";
            m_isUpdated = true;
            for (auto &p : properties) {
                if (p.first == "resource") {
                    auto pathName = p.second;
                    auto plain = QLatin1String("plain:");
                    if (pathName.startsWith(plain)) {
                        pathName = pathName.mid(plain.size());
                    }
                    if (QFileInfo(pathName).isRelative()) {
                        QDir projectDir(QFileInfo(m_tempFile->fileName()).dir());
                        pathName = projectDir.filePath(pathName);
                    }
                    QFile file(pathName);
                    if (file.open(QIODevice::ReadOnly)) {
                        p.first = "html";
                        p.second = QString::fromUtf8(file.readAll());
                    }
                    break;
                }
            }
            for (auto &p : properties) {
                if (p.first == "mlt_service") {
                    p.second = "qtext";
                    break;
                }
            }
            properties << MltProperty("shotcut:filter", "richText");
        }
    }
}

void MltXmlChecker::checkForProxy(const QString &mlt_service,
                                  QVector<MltXmlChecker::MltProperty> &properties)
{
    bool isTimewarp = mlt_service == "timewarp";
    if (mlt_service.startsWith("avformat") || isTimewarp) {
        QString resource;
        QString hash;
        QString speed = "1";
        for (auto &p : properties) {
            if ((!isTimewarp && p.first == "resource") || p.first == "warp_resource") {
                QFileInfo info(p.second);
                if (info.isRelative())
                    info.setFile(m_fileInfo.canonicalPath(), p.second);
                resource = info.filePath();
            } else if (p.first == kShotcutHashProperty) {
                hash = p.second;
            } else if (p.first == "warp_speed") {
                speed = p.second;
            } else if (p.first == kDisableProxyProperty && p.second == "1") {
                return;
            }
        }

        // Use GoPro LRV if available
        if (QFile::exists(ProxyManager::GoProProxyFilePath(resource))) {
            for (auto &p : properties) {
                if (p.first == "resource") {
                    p.second = ProxyManager::GoProProxyFilePath(resource);
                    if (isTimewarp) {
                        p.second = QString("%1:%2").arg(speed, p.second);
                    }
                    properties << MltProperty(kIsProxyProperty, "1");
                    properties << MltProperty(kMetaProxyProperty, "1");
                    properties << MltProperty(kOriginalResourceProperty, resource);
                    m_resource.notProxyMeta = !m_resource.isProxy;
                    m_isUpdated = true;
                    return;
                }
            }
        }

        QDir proxyDir(Settings.proxyFolder());
        QDir projectDir(QFileInfo(m_tempFile->fileName()).dir());
        QString fileName = hash + ProxyManager::videoFilenameExtension();
        projectDir.cd("proxies");
        if (proxyDir.exists(fileName) || projectDir.exists(fileName)) {
            ::utime(proxyDir.filePath(fileName).toUtf8().constData(), nullptr);
            ::utime(projectDir.filePath(fileName).toUtf8().constData(), nullptr);
            for (auto &p : properties) {
                if (p.first == "resource") {
                    if (projectDir.exists(fileName)) {
                        p.second = projectDir.filePath(fileName);
                    } else {
                        p.second = proxyDir.filePath(fileName);
                    }
                    if (isTimewarp) {
                        p.second = QString("%1:%2").arg(speed, p.second);
                    }
                    break;
                }
            }
            properties << MltProperty(kIsProxyProperty, "1");
            properties << MltProperty(kMetaProxyProperty, "1");
            properties << MltProperty(kOriginalResourceProperty, resource);
            m_resource.notProxyMeta = !m_resource.isProxy;
            m_isUpdated = true;
        }
    } else if ((mlt_service == "qimage" || mlt_service == "pixbuf")
               && !properties.contains(MltProperty(kShotcutSequenceProperty, "1"))) {
        QString resource;
        QString hash;
        for (auto &p : properties) {
            if (p.first == "resource") {
                QFileInfo info(p.second);
                if (info.isRelative())
                    info.setFile(m_fileInfo.canonicalPath(), p.second);
                resource = info.filePath();
            } else if (p.first == kShotcutHashProperty) {
                hash = p.second;
            } else if (p.first == kDisableProxyProperty && p.second == "1") {
                return;
            }
        }
        QDir proxyDir(Settings.proxyFolder());
        QDir projectDir(QFileInfo(m_tempFile->fileName()).dir());
        QString fileName = hash + ProxyManager::imageFilenameExtension();
        projectDir.cd("proxies");
        if (proxyDir.exists(fileName) || projectDir.exists(fileName)) {
            ::utime(proxyDir.filePath(fileName).toUtf8().constData(), nullptr);
            ::utime(projectDir.filePath(fileName).toUtf8().constData(), nullptr);
            for (auto &p : properties) {
                if (p.first == "resource") {
                    if (projectDir.exists(fileName)) {
                        p.second = projectDir.filePath(fileName);
                    } else {
                        p.second = proxyDir.filePath(fileName);
                    }
                    break;
                }
            }
            properties << MltProperty(kIsProxyProperty, "1");
            properties << MltProperty(kMetaProxyProperty, "1");
            properties << MltProperty(kOriginalResourceProperty, resource);
            m_resource.notProxyMeta = !m_resource.isProxy;
            m_isUpdated = true;
        }
    }
}

bool MltXmlChecker::checkMltVersion()
{
    if (m_mltVersion.majorVersion() > 7) {
        return false;
    }
    return true;
}
