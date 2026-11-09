/*
 * Copyright (c) 2026 Meltytech, LLC
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

#include "exportpreset.h"

#include <MltProfile.h>
#include <MltRepository.h>
#include <memory>
#include <QDir>
#include <QFileInfo>

namespace {

bool isHardwareEncoder(const QString &codec)
{
    return codec.endsWith("_amf") || codec.endsWith("_mf") || codec.endsWith("_nvenc")
           || codec.endsWith("_qsv") || codec.endsWith("_vaapi") || codec.endsWith("_videotoolbox");
}

bool presetBoolean(const QString &value, bool &result)
{
    const auto normalized = value.trimmed().toLower();
    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        result = true;
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        result = false;
        return true;
    }
    return false;
}

Mlt::Properties *stockPresets()
{
    return Mlt::Repository::presets();
}

Mlt::Properties *profiles()
{
    return Mlt::Profile::list();
}

QString stockPresetName(const QString &key, Mlt::Properties &presets, Mlt::Properties *profileList)
{
    Mlt::Properties preset((mlt_properties) presets.get_data(key.toLatin1().constData()));
    if (preset.get("meta.preset.name"))
        return QString::fromUtf8(preset.get("meta.preset.name"));
    QString name = key.mid(QStringLiteral("consumer/avformat/").size());
    auto parts = name.split('/');
    if (parts.size() > 1 && profileList
        && profileList->get_data(parts.first().toLatin1().constData())) {
        const QString profile = parts.takeFirst();
        return QStringLiteral("%1 (%2)").arg(parts.join('/'), profile);
    }
    return name;
}

QString stockPresetExtension(const QString &key, Mlt::Properties &presets)
{
    Mlt::Properties preset((mlt_properties) presets.get_data(key.toLatin1().constData()));
    return QString::fromLatin1(preset.get("meta.preset.extension"));
}

bool resolveProperties(const QString &id,
                       Mlt::Properties &properties,
                       QString &extension,
                       bool &twoPass,
                       QString &error,
                       const QString &customDirectory,
                       int audioChannels)
{
    if (id == "default") {
        properties.set("f", "mp4");
        properties.set("vcodec", "libx264");
        properties.set("acodec", "aac");
        properties.set("preset", "fast");
        properties.set("crf", 23);
        properties.set("movflags", "+faststart");
        properties.set("ar", "48000");
        const int channels = audioChannels;
        if (channels == 1 || channels == 2 || channels == 4 || channels == 6) {
            properties.set("channels", channels);
            properties.set("channel_layout", channels == 4 ? "quad" : "auto");
        }
        properties.set("real_time", -1);
        extension = "mp4";
        return true;
    }

    const bool custom = id.startsWith("custom:");
    const QString key = id.mid(custom ? 7 : 6);
    Mlt::Properties preset;
    if (custom) {
        const QFileInfo keyInfo(key);
        const QDir directory(customDirectory);
        const QStringList entries = directory.entryList(QDir::Files | QDir::NoDotAndDotDot
                                                        | QDir::Readable);
        if (key.isEmpty() || keyInfo.fileName() != key || !entries.contains(key)) {
            error = QObject::tr("Custom export preset does not exist: %1").arg(key);
            return false;
        }
        const QString path = directory.filePath(key);
        preset.load(path.toUtf8().constData());
        if (preset.count() == 0) {
            error = QObject::tr("Export preset is empty or invalid: %1").arg(path);
            return false;
        }
    } else {
        const QString prefix = QStringLiteral("consumer/avformat/");
        if (!id.startsWith("stock:") || !key.startsWith(prefix)) {
            error = QObject::tr("Export preset does not exist: %1").arg(id);
            return false;
        }
        std::unique_ptr<Mlt::Properties> available(stockPresets());
        if (!available || !available->is_valid()
            || !available->get_data(key.toLatin1().constData())) {
            error = QObject::tr("Stock export preset does not exist: %1").arg(key);
            return false;
        }
        Mlt::Properties source((mlt_properties) available->get_data(key.toLatin1().constData()));
        if (source.get_int("meta.preset.hidden")) {
            error = QObject::tr("Stock export preset is hidden: %1").arg(key);
            return false;
        }
        preset = source;

        const auto parts = key.mid(prefix.size()).split('/');
        if (parts.size() > 1) {
            std::unique_ptr<Mlt::Properties> profileList(profiles());
            if (profileList && profileList->get_data(parts.at(0).toLatin1().constData())) {
                Mlt::Profile profile(parts.at(0).toLatin1().constData());
                properties.set("width", profile.width());
                properties.set("height", profile.height());
                properties.set("aspect", profile.dar());
                properties.set("progressive", profile.progressive());
                properties.set("frame_rate_num", profile.frame_rate_num());
                properties.set("frame_rate_den", profile.frame_rate_den());
            }
        }
    }

    extension = QString::fromLatin1(preset.get("meta.preset.extension"));
    if (!ExportPreset::resolve(preset, properties, twoPass, error))
        return false;
    if (!properties.get("real_time"))
        properties.set("real_time", -1);
    return true;
}

} // namespace

bool ExportPreset::isImageSequence(Mlt::Properties &properties)
{
    if (properties.get_int("vn") || properties.get_int("video_off"))
        return false;
    const QString codec = QString::fromLatin1(properties.get("vcodec"));
    const QString format = QString::fromLatin1(properties.get("f"));
    return format == "image2" || codec == "bmp" || codec == "dpx" || codec == "png"
           || codec == "ppm" || codec == "targa" || codec == "tiff";
}

QVariantList ExportPreset::list(const QString &customDirectory)
{
    QVariantList result;
    result.append(QVariantMap{{"id", "default"},
                              {"name", QObject::tr("Default")},
                              {"category", QObject::tr("Stock")},
                              {"custom", false},
                              {"extension", "mp4"}});

    QDir directory(customDirectory);
    if (directory.exists()) {
        const auto entries = directory.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable,
                                                 QDir::Name);
        for (const auto &entry : entries) {
            Mlt::Properties preset;
            preset.load(directory.filePath(entry).toUtf8().constData());
            QString displayName = entry;
            QString category = QObject::tr("Custom");
            const auto parts = entry.split(')');
            if (parts.size() > 1 && !parts.at(1).isEmpty() && !entry.contains('(')) {
                category += '/' + parts.first();
                displayName = parts.mid(1).join(')');
            }
            result.append(QVariantMap{{"id", "custom:" + entry},
                                      {"name", displayName},
                                      {"category", category},
                                      {"custom", true},
                                      {"extension",
                                       QString::fromLatin1(preset.get("meta.preset.extension"))}});
        }
    }

    std::unique_ptr<Mlt::Properties> presets(stockPresets());
    std::unique_ptr<Mlt::Properties> profileList(profiles());
    const QString prefix = QStringLiteral("consumer/avformat/");
    if (presets && presets->is_valid()) {
        for (int i = 0; i < presets->count(); ++i) {
            const QString fullKey = QString::fromLatin1(presets->get_name(i));
            if (!fullKey.startsWith(prefix))
                continue;
            Mlt::Properties preset(
                (mlt_properties) presets->get_data(fullKey.toLatin1().constData()));
            if (preset.get_int("meta.preset.hidden"))
                continue;
            const QString name = stockPresetName(fullKey, *presets, profileList.get());
            QString category = QObject::tr("Stock");
            QString displayName = name;
            const int slash = name.lastIndexOf('/');
            if (slash >= 0) {
                category += '/' + name.left(slash);
                displayName = name.mid(slash + 1);
            }
            result.append(QVariantMap{{"id", "stock:" + fullKey},
                                      {"name", displayName},
                                      {"category", category},
                                      {"custom", false},
                                      {"extension", stockPresetExtension(fullKey, *presets)}});
        }
    }
    return result;
}

bool ExportPreset::resolve(const QString &id,
                           ResolvedPreset &result,
                           QString &error,
                           const QString &customDirectory,
                           int audioChannels)
{
    Mlt::Properties properties;
    if (!resolveProperties(id,
                           properties,
                           result.extension,
                           result.twoPass,
                           error,
                           customDirectory,
                           audioChannels))
        return false;
    result.imageSequence = isImageSequence(properties);
    result.properties = properties;
    return true;
}

bool ExportPreset::resolve(Mlt::Properties &stored,
                           Mlt::Properties &consumer,
                           bool &twoPass,
                           QString &error)
{
    bool v2passPresent = false;
    bool v2pass = false;
    bool passPresent = false;
    int pass = 0;
    for (int i = 0; i < stored.count(); ++i) {
        const char *rawName = stored.get_name(i);
        if (!rawName || !*rawName)
            continue;
        const QString name = QString::fromLatin1(rawName);
        const QString value = QString::fromUtf8(stored.get(rawName));
        if (name == "v2pass") {
            v2passPresent = true;
            if (!presetBoolean(value, v2pass)) {
                error = QObject::tr("Invalid v2pass value in preset: %1").arg(value);
                return false;
            }
        } else if (name == "pass") {
            passPresent = true;
            bool ok = false;
            pass = value.toInt(&ok);
            if (!ok || (pass != 0 && pass != 1 && pass != 2)) {
                error = QObject::tr("Preset pass must be 0, 1, or 2: %1").arg(value);
                return false;
            }
        } else if (!name.startsWith("meta.preset.") && !name.startsWith('_')) {
            consumer.set(rawName, stored.get(rawName));
        }
    }
    if (v2passPresent && passPresent && v2pass != (pass == 1 || pass == 2)) {
        error = QObject::tr("Preset v2pass and pass settings conflict.");
        return false;
    }
    twoPass = v2pass || pass == 1 || pass == 2;
    if (twoPass && (consumer.get_int("vn") || consumer.get_int("video_off"))) {
        error = QObject::tr("Two-pass export requires video output to remain enabled.");
        return false;
    }
    if (twoPass && isHardwareEncoder(QString::fromLatin1(consumer.get("vcodec")))) {
        error = QObject::tr("This hardware encoder preset does not support API two-pass export.");
        return false;
    }
    return true;
}

void ExportPreset::configurePass(QDomElement &consumer,
                                 const QString &target,
                                 const QString &videoCodec,
                                 int pass,
                                 bool preservePresetProperties)
{
    if (videoCodec == "libx265") {
        if (pass == 1 || pass == 2) {
            const QString stats = QString(target).replace(":", "\\:") + "_2pass.log";
            consumer.setAttribute("x265-params",
                                  QStringLiteral("pass=%1:stats=%2:%3")
                                      .arg(pass)
                                      .arg(stats, consumer.attribute("x265-params")));
        }
    } else if (videoCodec == "libsvtav1") {
        if (pass == 1 || pass == 2) {
            QStringList params{QStringLiteral("passes=2"),
                               QStringLiteral("pass=%1").arg(pass),
                               QStringLiteral("stats=%1")
                                   .arg(QString(target).replace(":", "\\:") + "_2pass.log")};
            const QString original = consumer.attribute("svtav1-params");
            if (!original.isEmpty())
                params.append(original);
            consumer.setAttribute("svtav1-params", params.join(':'));
        }
    } else if (pass == 1 || pass == 2) {
        consumer.setAttribute("pass", pass);
        consumer.setAttribute("passlogfile", target + "_2pass.log");
    }

    if (pass == 1 && videoCodec != "libx265" && videoCodec != "libsvtav1") {
        consumer.setAttribute("fastfirstpass", 1);
        consumer.removeAttribute("acodec");
        consumer.setAttribute("an", 1);
    } else if (videoCodec != "libx265" && videoCodec != "libsvtav1"
               && (pass == 2 || !preservePresetProperties)) {
        consumer.removeAttribute("fastfirstpass");
    }
}
