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

#include "filteranimation.h"

#include <MltAnimation.h>
#include <QSet>

#include <cmath>

namespace Shotcut {
namespace Api {
namespace FilterAnimation {
namespace {
const QStringList types{"discrete",      "linear",         "smooth",           "smoothNatural",
                        "smoothTight",   "sinusoidalIn",   "sinusoidalOut",    "sinusoidalInOut",
                        "quadraticIn",   "quadraticOut",   "quadraticInOut",   "cubicIn",
                        "cubicOut",      "cubicInOut",     "quarticIn",        "quarticOut",
                        "quarticInOut",  "quinticIn",      "quinticOut",       "quinticInOut",
                        "exponentialIn", "exponentialOut", "exponentialInOut", "circularIn",
                        "circularOut",   "circularInOut",  "backIn",           "backOut",
                        "backInOut",     "elasticIn",      "elasticOut",       "elasticInOut",
                        "bounceIn",      "bounceOut",      "bounceInOut"};

bool number(const QVariant &value)
{
    const auto type = value.metaType().id();
    return (type == QMetaType::Int || type == QMetaType::UInt || type == QMetaType::LongLong
            || type == QMetaType::ULongLong || type == QMetaType::Double
            || type == QMetaType::Float)
           && std::isfinite(value.toDouble());
}

QVariant valueAt(
    Mlt::Properties &properties, const QByteArray &name, bool rectangle, int frame, int duration)
{
    if (!rectangle)
        return properties.anim_get_double(name.constData(), frame, duration);
    const auto rect = properties.anim_get_rect(name.constData(), frame, duration);
    return QVariantMap{{"x", rect.x},
                       {"y", rect.y},
                       {"width", rect.w},
                       {"height", rect.h},
                       {"opacity", rect.o}};
}
} // namespace

QVariantList read(Mlt::Properties &properties, const QString &name, bool rectangle, int duration)
{
    const auto key = name.toUtf8();
    if (!properties.property_exists(key.constData()))
        return {};
    valueAt(properties, key, rectangle, 0, duration);
    Mlt::Animation animation(properties.get_animation(key.constData()));
    QVariantList result;
    for (int i = 0; animation.is_valid() && i < animation.key_count(); ++i) {
        const int frame = animation.key_get_frame(i);
        result.append(QVariantMap{{"frame", frame},
                                  {"value", valueAt(properties, key, rectangle, frame, duration)},
                                  {"interpolation", types.value(int(animation.key_get_type(i)))}});
    }
    return result;
}

bool write(Mlt::Properties &properties,
           const QString &name,
           bool rectangle,
           int duration,
           const QVariantList &keys,
           QString &error)
{
    error.clear();
    if (keys.isEmpty() || keys.size() > 10000 || duration <= 0) {
        error = QObject::tr("Give between 1 and 10000 keyframes and a positive filter duration.");
        return false;
    }
    Mlt::Properties values;
    QSet<int> frames;
    const auto key = name.toUtf8();
    for (const auto &entry : keys) {
        const auto item = entry.toMap();
        const auto frameValue = item.value("frame");
        const double frame = frameValue.toDouble();
        const int type = types.indexOf(item.value("interpolation", "linear").toString());
        if (entry.metaType().id() != QMetaType::QVariantMap || item.size() < 2 || item.size() > 3
            || !item.contains("value") || !number(frameValue) || frame < 0 || frame >= duration
            || std::floor(frame) != frame || frames.contains(int(frame)) || type < 0
            || (item.size() == 3 && !item.contains("interpolation"))) {
            error = QObject::tr("Keyframes need unique integer frames within the filter, a value, "
                                "and an optional supported interpolation name.");
            return false;
        }
        frames.insert(int(frame));
        int status = 0;
        if (rectangle) {
            const auto rect = item.value("value").toMap();
            const QStringList fields{"x", "y", "width", "height", "opacity"};
            bool valid = rect.size() == fields.size();
            for (const auto &field : fields)
                valid = valid && number(rect.value(field));
            if (!valid) {
                error = QObject::tr(
                    "Rectangle keyframes require finite x, y, width, height, opacity.");
                return false;
            }
            const mlt_rect value{rect.value("x").toDouble(),
                                 rect.value("y").toDouble(),
                                 rect.value("width").toDouble(),
                                 rect.value("height").toDouble(),
                                 rect.value("opacity").toDouble()};
            status = values.anim_set(key.constData(),
                                     value,
                                     int(frame),
                                     duration,
                                     mlt_keyframe_type(type));
        } else {
            if (!number(item.value("value"))) {
                error = QObject::tr("Numeric keyframes require finite numeric values.");
                return false;
            }
            status = values.anim_set(key.constData(),
                                     item.value("value").toDouble(),
                                     int(frame),
                                     duration,
                                     mlt_keyframe_type(type));
        }
        if (status) {
            error = QObject::tr("MLT could not create the keyframe.");
            return false;
        }
    }
    properties.set(key.constData(), values.get(key.constData()));
    return true;
}
} // namespace FilterAnimation
} // namespace Api
} // namespace Shotcut
