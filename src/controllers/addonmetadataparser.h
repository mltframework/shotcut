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

#ifndef ADDONMETADATAPARSER_H
#define ADDONMETADATAPARSER_H

#include <MltProperties.h>
#include <QList>
#include <QString>
#include <QStringList>

struct AddOnParameterDescriptor
{
    QString name;
    QString title;
    QString type;
    bool isReadOnly = false;
    bool supportsKeyframes = false;
    QStringList values;
    QString defaultValue;
    QString unit;
    QString minimum;
    QString maximum;
    QString description;
};

struct AddOnFilterDescriptor
{
    QString service;
    QString title;
    QString description;
    bool isAudio = false;
    QStringList imageFormats;
    QList<AddOnParameterDescriptor> parameters;
};

class AddOnMetadataParser
{
public:
    static AddOnFilterDescriptor parse(const QString &service, Mlt::Properties *mltMetadata);
};

#endif // ADDONMETADATAPARSER_H
