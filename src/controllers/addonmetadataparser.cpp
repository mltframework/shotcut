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

#include "addonmetadataparser.h"

static bool parseYesNoBool(const char *value)
{
    if (!value)
        return false;
    const QString text = QString::fromUtf8(value).trimmed().toLower();
    return text == QStringLiteral("yes") || text == QStringLiteral("true")
           || text == QStringLiteral("1");
}

AddOnFilterDescriptor AddOnMetadataParser::parse(const QString &service,
                                                 Mlt::Properties *mltMetadata)
{
    AddOnFilterDescriptor descriptor;
    descriptor.service = service;
    descriptor.title = service;

    if (!mltMetadata || !mltMetadata->is_valid())
        return descriptor;

    descriptor.title = QString::fromUtf8(mltMetadata->get("title"));
    if (descriptor.title.isEmpty())
        descriptor.title = service;

    descriptor.description = QString::fromUtf8(mltMetadata->get("description"));

    Mlt::Properties imageFormats(mltMetadata->get_data("image_formats"));
    if (imageFormats.is_valid()) {
        for (int i = 0; i < imageFormats.count(); ++i)
            descriptor.imageFormats << QString::fromUtf8(imageFormats.get(i));
    }

    Mlt::Properties tags(mltMetadata->get_data("tags"));
    if (tags.is_valid()) {
        for (int i = 0; i < tags.count(); ++i) {
            if (!qstricmp(tags.get(i), "Audio")) {
                descriptor.isAudio = true;
                break;
            }
        }
    }

    Mlt::Properties parameters(mltMetadata->get_data("parameters"));
    if (!parameters.is_valid())
        return descriptor;

    for (int i = 0; i < parameters.count(); ++i) {
        const char *rawName = parameters.get_name(i);
        Mlt::Properties parameter(rawName ? parameters.get_data(rawName) : nullptr);

        AddOnParameterDescriptor out;
        if (parameter.is_valid()) {
            out.title = QString::fromUtf8(parameter.get("title"));
            out.defaultValue = QString::fromUtf8(parameter.get("default"));
            out.type = QString::fromUtf8(parameter.get("type"));
            out.isReadOnly = parseYesNoBool(parameter.get("readonly"));
            out.supportsKeyframes = parseYesNoBool(parameter.get("animation"));
            out.unit = QString::fromUtf8(parameter.get("unit"));
            out.minimum = QString::fromUtf8(parameter.get("minimum"));
            out.maximum = QString::fromUtf8(parameter.get("maximum"));
            out.description = QString::fromUtf8(parameter.get("description"));
            out.name = QString::fromUtf8(parameter.get("identifier"));

            Mlt::Properties values(parameter.get_data("values"));
            if (values.is_valid()) {
                for (int v = 0; v < values.count(); ++v) {
                    QString option = QString::fromUtf8(values.get(v));
                    if (option.isEmpty()) {
                        const char *optionName = values.get_name(v);
                        if (optionName)
                            option = QString::fromUtf8(optionName);
                    }
                    if (!option.isEmpty() && !out.values.contains(option))
                        out.values << option;
                }
            }
        }

        if (out.name.isEmpty() && rawName)
            out.name = QString::fromUtf8(rawName);

        descriptor.parameters.push_back(out);
    }

    return descriptor;
}
