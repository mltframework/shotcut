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

#ifndef SHOTCUT_API_APIFILTERS_H
#define SHOTCUT_API_APIFILTERS_H

#include "api/apiobject.h"

#include <MltProducer.h>
#include <QVariantList>

class QmlMetadata;

namespace Shotcut {
namespace Api {

class ApiFilters : public ApiObject
{
    Q_OBJECT
    Q_CLASSINFO("doc",
                "Filters, keyframes, and analysis on timeline clips, tracks, and main output. A "
                "target is exactly "
                "one of {clipId: string}, {trackIndex: integer}, or {output: true}. Indexes are "
                "Filters panel rows, including links; re-read after add, remove, or move.")

public:
    explicit ApiFilters(QObject *parent = nullptr);

    Q_CLASSINFO("mcp.available", "read")
    Q_CLASSINFO("doc.available",
                "Lists installed Shotcut filter metadata IDs, names, native services, and target "
                "restrictions. Only regular filters are supported, not links or filter sets.")
    Q_INVOKABLE QVariantList available() const;

    Q_CLASSINFO("mcp.metadata", "read")
    Q_CLASSINFO("doc.metadata",
                "Returns Shotcut metadata plus native MLT metadata for a metadata ID. Native "
                "parameter fields (identifier, type, default, minimum, maximum, animation, "
                "readonly, values, etc.) are returned as declared strings, without UI scaling. "
                "Missing fields stay absent; metadata may be incomplete or unavailable.")
    Q_CLASSINFO("arg.metadata.metadataId",
                "Shotcut metadata ID from available(), not an arbitrary service.")
    Q_INVOKABLE QVariant metadata(const QString &metadataId) const;

    Q_CLASSINFO("mcp.list", "read")
    Q_CLASSINFO("doc.list",
                "Lists regular user filters with index, metadataId, name, enabled, inclusive "
                "source in/out, parameters, and readOnly. Excludes internal filters and links; "
                "indexes still include user links. Track volume filters are read-only here.")
    Q_CLASSINFO("arg.list.target", "Exactly one clipId, trackIndex, or output: true.")
    Q_INVOKABLE QVariantList list(const QVariantMap &target) const;

    Q_CLASSINFO("mcp.add", "edit")
    Q_CLASSINFO("doc.add",
                "Adds a filter by Shotcut metadata ID and returns its snapshot. Uses native MLT "
                "defaults plus supplied static parameters, without loading the QML filter UI. "
                "Job-based video analysis filters start with analyze disabled to avoid preview "
                "file writes. "
                "Defaults can differ from the Filters panel. Enforces metadata target, GPU, "
                "and duplicate restrictions. Inserts in Shotcut's normal filter order. Clip "
                "ranges include adjacent transition footage. If the target is displayed in the "
                "Filters panel, the added filter is selected as with a normal UI addition.")
    Q_CLASSINFO("arg.add.target", "Exactly one clipId, trackIndex, or output: true.")
    Q_CLASSINFO("arg.add.metadataId", "Shotcut metadata ID from available().")
    Q_CLASSINFO("arg.add.parameters",
                "Native property names mapped to static values; defaults to {}.")
    Q_INVOKABLE QVariant add(const QVariantMap &target,
                             const QString &metadataId,
                             const QVariantMap &parameters = QVariantMap());

    Q_CLASSINFO("mcp.remove", "edit")
    Q_CLASSINFO("doc.remove", "Removes one regular user filter and returns its previous snapshot.")
    Q_CLASSINFO("arg.remove.target", "Exactly one clipId, trackIndex, or output: true.")
    Q_CLASSINFO("arg.remove.index", "Filters panel row from list().")
    Q_INVOKABLE QVariant remove(const QVariantMap &target, int index);

    Q_CLASSINFO("mcp.move", "edit")
    Q_CLASSINFO("doc.move",
                "Reorders a regular filter to another regular filter's row and returns it.")
    Q_CLASSINFO("arg.move.target", "Exactly one clipId, trackIndex, or output: true.")
    Q_CLASSINFO("arg.move.index", "Current Filters panel row.")
    Q_CLASSINFO("arg.move.toIndex", "Final Filters panel row; cannot be a link or track volume row.")
    Q_INVOKABLE QVariant move(const QVariantMap &target, int index, int toIndex);

    Q_CLASSINFO("mcp.setEnabled", "edit")
    Q_CLASSINFO("doc.setEnabled", "Enables or disables a regular filter and returns its snapshot.")
    Q_CLASSINFO("arg.setEnabled.target", "Exactly one clipId, trackIndex, or output: true.")
    Q_CLASSINFO("arg.setEnabled.index", "Filters panel row from list().")
    Q_CLASSINFO("arg.setEnabled.enabled", "True to enable, false to disable.")
    Q_INVOKABLE QVariant setEnabled(const QVariantMap &target, int index, bool enabled);

    Q_CLASSINFO("mcp.setParameters", "edit")
    Q_CLASSINFO(
        "doc.setParameters",
        "Sets native static parameters and returns the filter. Values are finite numbers, "
        "booleans, strings, or rectangles {x,y,width,height,opacity}. Declared MLT types "
        "and readonly flags are checked where available. Undeclared native properties "
        "are allowed; internal identity, timing, and enable properties are not. Cannot "
        "write animation strings or change already animated parameters; use setKeyframes. Omitted "
        "properties stay unchanged. Does not select the target or seek.")
    Q_CLASSINFO("arg.setParameters.target", "Exactly one clipId, trackIndex, or output: true.")
    Q_CLASSINFO("arg.setParameters.index", "Filters panel row from list().")
    Q_CLASSINFO("arg.setParameters.parameters",
                "Native property names and static values, not UI percentages.")
    Q_INVOKABLE QVariant setParameters(const QVariantMap &target,
                                       int index,
                                       const QVariantMap &parameters);

    Q_CLASSINFO("mcp.keyframes", "read")
    Q_CLASSINFO(
        "doc.keyframes",
        "Returns {parameter, rectangle, duration, keys}. Keys have frame, value, "
        "and interpolation. Frames are relative to the filter start. Supports Shotcut-declared "
        "numeric and rectangle keyframe parameters, in native units; no UI scaling or ganging.")
    Q_CLASSINFO("arg.keyframes.target", "Exactly one clipId, trackIndex, or output: true.")
    Q_CLASSINFO("arg.keyframes.index", "Filters panel row from list().")
    Q_CLASSINFO("arg.keyframes.parameter",
                "Keyframe property name from metadata().keyframeParameters.")
    Q_INVOKABLE QVariant keyframes(const QVariantMap &target,
                                   int index,
                                   const QString &parameter) const;

    Q_CLASSINFO("mcp.setKeyframes", "edit")
    Q_CLASSINFO(
        "doc.setKeyframes",
        "Replaces one parameter's curve with 1 to 10000 structured keys; returns "
        "keyframes(). Each key is {frame, value, interpolation?}, with a unique integer frame "
        "within the filter duration. Numeric values or rectangles {x,y,width,height,opacity}. "
        "Interpolation defaults to linear; see the guide for supported names. Disables simple "
        "animate-in/out mode; other parameter curves stay unchanged. Undoable; does not seek or "
        "select.")
    Q_CLASSINFO("arg.setKeyframes.target", "Exactly one clipId, trackIndex, or output: true.")
    Q_CLASSINFO("arg.setKeyframes.index", "Filters panel row.")
    Q_CLASSINFO("arg.setKeyframes.parameter", "Supported numeric or rectangle keyframe property.")
    Q_CLASSINFO("arg.setKeyframes.keys", "Array of structured keyframes in filter-relative frames.")
    Q_INVOKABLE QVariant setKeyframes(const QVariantMap &target,
                                      int index,
                                      const QString &parameter,
                                      const QVariantList &keys);

    Q_CLASSINFO("mcp.removeKeyframes", "edit")
    Q_CLASSINFO("doc.removeKeyframes",
                "Removes keys at the supplied filter-relative frames and returns "
                "keyframes(). Every requested frame must exist. Removing all keys leaves the "
                "earliest key's value static. Undoable; disables simple animate-in/out mode.")
    Q_CLASSINFO("arg.removeKeyframes.target", "Exactly one clipId, trackIndex, or output: true.")
    Q_CLASSINFO("arg.removeKeyframes.index", "Filters panel row.")
    Q_CLASSINFO("arg.removeKeyframes.parameter", "Supported numeric or rectangle keyframe property.")
    Q_CLASSINFO("arg.removeKeyframes.frames", "Array of unique existing integer keyframe positions.")
    Q_INVOKABLE QVariant removeKeyframes(const QVariantMap &target,
                                         int index,
                                         const QString &parameter,
                                         const QVariantList &frames);

    Q_CLASSINFO("mcp.analyze", "action")
    Q_CLASSINFO(
        "doc.analyze",
        "Starts selection-independent analysis for loudness, vidstab, or "
        "opencv.tracker and returns {jobId}. Launch outside edit transactions. Existing "
        "filter settings are used; stabilization requires an absolute, nonexistent filename "
        "set beforehand. Completion automatically applies results without creating an undo step, "
        "including during active edit transactions. "
        "jobs.job reports analysisState and analysisError in addition to process status. "
        "Generated external files are not undone or deleted.")
    Q_CLASSINFO("arg.analyze.target", "Exactly one clipId, trackIndex, or output: true.")
    Q_CLASSINFO("arg.analyze.index", "Filters panel row of a supported analysis filter.")
    Q_INVOKABLE QVariant analyze(const QVariantMap &target, int index);

signals:
    void changed();
    void analysisChanged();

private:
    struct Target
    {
        Mlt::Producer producer;
        QString kind;
        int in = 0;
        int out = 0;
    };
    bool resolveTarget(const QVariantMap &target, Target &result, bool writable) const;
    QmlMetadata *filterMetadata(const QString &id) const;
    QVariantMap metadataSnapshot(QmlMetadata *meta) const;
    QVariantMap filterSnapshot(Mlt::Producer &producer, int index) const;
    bool checkFilter(Mlt::Producer &producer, int index, Mlt::Service &service, bool writable) const;
    bool prepareParameters(Mlt::Service &service,
                           const QVariantMap &parameters,
                           Mlt::Properties &values) const;
    bool animationParameter(Mlt::Service &service, const QString &parameter, bool &rectangle) const;
    QVariant applyParameters(Target &target, int index, Mlt::Properties &values);
    QVariant editFailed() const;
};

} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_APIFILTERS_H
