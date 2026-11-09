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

#ifndef SHOTCUT_MCP_NO_DEFAULT_TOOLS
#include "mcp/mcpbridge.h"
#endif

#include <QJsonArray>
#include <QJsonObject>

namespace Shotcut {
namespace Mcp {

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

ToolRegistry createShotcutToolRegistry()
{
    ToolRegistry registry;
    registry.registerTool(makeForumSearchTool());
    registry.registerTool(makeDocumentationSearchTool());
#ifndef SHOTCUT_MCP_NO_DEFAULT_TOOLS
    for (const auto &tool : makeApiTools())
        registry.registerTool(tool);
#endif
    return registry;
}

} // namespace Mcp
} // namespace Shotcut
