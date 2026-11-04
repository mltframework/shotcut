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

#include "mcp/shotcuttools.h"

#include "mcp/projectinspector.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

namespace Shotcut {
namespace Mcp {

QJsonObject shotcutProjectInspectTool(const QJsonObject &arguments)
{
    Q_UNUSED(arguments);

    QJsonObject result = inspectProject();
    result["tool"] = QStringLiteral("shotcut_project_inspect");
    result["status"] = QStringLiteral("ok");
    return result;
}

QJsonObject shotcutTimelineSummaryTool(const QJsonObject &arguments)
{
    QJsonObject result;
    const QJsonObject project = inspectProject();
    const bool includeTracks = arguments.value("include_tracks").toBool();

    result["tool"] = QStringLiteral("shotcut_timeline_summary");
    result["status"] = QStringLiteral("ok");
    result["include_tracks"] = includeTracks;
    result["active_context"] = project.value("active_context");
    result["has_timeline"] = project.value("has_timeline");
    result["has_playlist"] = project.value("has_playlist");

    if (project.value("has_timeline").toBool()) {
        QJsonObject timeline = project.value("timeline").toObject();
        if (!includeTracks)
            timeline.remove("tracks");
        result["timeline"] = timeline;
    }

    if (project.value("has_playlist").toBool()) {
        const QJsonObject playlist = project.value("playlist").toObject();
        result["playlist"] = playlist;
    }

    result["current_producer"] = project.value("current_producer");
    result["note"] = QStringLiteral("This summary is derived from the live Shotcut project state "
                                    "and tracks the current timeline/playlist context.");
    return result;
}

ToolDefinition makeProjectInspectTool()
{
    ToolDefinition tool;
    tool.name = QStringLiteral("shotcut_project_inspect");
    tool.description = QStringLiteral(
        "Inspect the open Shotcut project, timeline, selection, and current producer state.");
    tool.parameters.insert("type", QStringLiteral("object"));
    tool.parameters.insert("properties", QJsonObject());
    tool.parameters.insert("required", QJsonArray());
    tool.handler = [](const QJsonObject &arguments) { return shotcutProjectInspectTool(arguments); };
    return tool;
}

ToolDefinition makeForumSearchTool()
{
    ToolDefinition tool;
    tool.name = QStringLiteral("shotcut_forum_search");
    tool.description = QStringLiteral(
        "Search Shotcut user discussions, excluding English documentation, for troubleshooting, "
        "reported problems, and workarounds. Prefer shotcut_documentation_search for instructions "
        "on using Shotcut.");

    QJsonObject properties;
    QJsonObject querySchema;
    querySchema.insert("type", QStringLiteral("string"));
    querySchema.insert("description", QStringLiteral("Search keywords or a question."));
    properties.insert("query", querySchema);

    QJsonArray required;
    required.append(QStringLiteral("query"));

    tool.parameters.insert("type", QStringLiteral("object"));
    tool.parameters.insert("properties", properties);
    tool.parameters.insert("required", required);
    return tool;
}

ToolDefinition makeDocumentationSearchTool()
{
    auto tool = makeForumSearchTool();
    tool.name = QStringLiteral("shotcut_documentation_search");
    tool.description = QStringLiteral(
        "Search Shotcut's English documentation for instructions on features, controls, filters, "
        "and keyboard shortcuts. Use this first for questions about how to use Shotcut.");
    return tool;
}

ToolDefinition makeTimelineSummaryTool()
{
    ToolDefinition tool;
    tool.name = QStringLiteral("shotcut_timeline_summary");
    tool.description = QStringLiteral(
        "Produce a concise summary of the current timeline and selected clips.");

    QJsonObject properties;
    QJsonObject includeTracksSchema;
    includeTracksSchema.insert("type", QStringLiteral("boolean"));
    includeTracksSchema.insert("description",
                               QStringLiteral(
                                   "Include per-track details. Defaults to false when omitted."));
    properties.insert("include_tracks", includeTracksSchema);

    tool.parameters.insert("type", QStringLiteral("object"));
    tool.parameters.insert("properties", properties);
    tool.parameters.insert("required", QJsonArray());
    tool.handler = [](const QJsonObject &arguments) {
        return shotcutTimelineSummaryTool(arguments);
    };
    return tool;
}

ToolRegistry createShotcutToolRegistry()
{
    ToolRegistry registry;
    registry.registerTool(makeProjectInspectTool());
    registry.registerTool(makeForumSearchTool());
    registry.registerTool(makeDocumentationSearchTool());
    registry.registerTool(makeTimelineSummaryTool());
    return registry;
}

} // namespace Mcp
} // namespace Shotcut
