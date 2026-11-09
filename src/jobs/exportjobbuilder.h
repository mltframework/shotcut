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

#ifndef EXPORTJOBBUILDER_H
#define EXPORTJOBBUILDER_H

#include <MltProperties.h>
#include <QString>
#include <QThread>

class MeltJob;
namespace Mlt {
class Producer;
}

struct ExportJobOptions
{
    int realtime = -1;
    int pass = 0;
    QThread::Priority priority = QThread::NormalPriority;
    bool includeSubtitles = true;
    bool adjustClipFilterRanges = false;
    bool useProxy = false;
    bool strictFormatCompatibility = false;
    bool useMultiConsumer = false;
    bool useMultiConsumerForReframe = false;
    bool validateProjectXml = false;
    bool preservePresetProperties = false;
    bool useHardwareDecoder = false;
    int in = -1;
    int out = -1;
};

namespace ExportJobBuilder {
// Returns nullptr with a nonempty error on failure; clears error on success.
MeltJob *create(Mlt::Producer *service,
                const QString &target,
                Mlt::Properties &properties,
                const ExportJobOptions &options,
                QString &error);
} // namespace ExportJobBuilder

#endif // EXPORTJOBBUILDER_H
