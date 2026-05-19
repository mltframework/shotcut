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

#ifndef ADDONQMLGENERATOR_H
#define ADDONQMLGENERATOR_H

#include "addonmetadataparser.h"

#include <QDir>
#include <QString>

class AddOnQmlGenerator
{
public:
    bool generate(const AddOnFilterDescriptor &descriptor,
                  const QDir &outputDir,
                  const QString &qmlFileName,
                  QString *errorMessage = nullptr) const;
};

#endif // ADDONQMLGENERATOR_H
