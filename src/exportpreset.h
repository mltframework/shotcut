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

#ifndef EXPORTPRESET_H
#define EXPORTPRESET_H

#include <MltProperties.h>
#include <QDomElement>
#include <QString>
#include <QVariantList>

namespace ExportPreset {
struct ResolvedPreset
{
    Mlt::Properties properties;
    QString extension;
    bool twoPass = false;
    bool imageSequence = false;
};

QVariantList list(const QString &customDirectory);
bool isImageSequence(Mlt::Properties &properties);
bool resolve(const QString &id,
             ResolvedPreset &result,
             QString &error,
             const QString &customDirectory,
             int audioChannels);
bool resolve(Mlt::Properties &stored, Mlt::Properties &consumer, bool &twoPass, QString &error);
void configurePass(QDomElement &consumer,
                   const QString &target,
                   const QString &videoCodec,
                   int pass,
                   bool preservePresetProperties);
} // namespace ExportPreset

#endif // EXPORTPRESET_H
