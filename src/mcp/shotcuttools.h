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

#ifndef SHOTCUT_MCP_SHOTCUTTOOLS_H
#define SHOTCUT_MCP_SHOTCUTTOOLS_H

#include "mcp/tooldefinition.h"
#include "mcp/toolregistry.h"

#include <QJsonObject>

namespace Shotcut {
namespace Mcp {

QJsonObject shotcutProjectInspectTool(const QJsonObject &arguments);
QJsonObject shotcutTimelineSummaryTool(const QJsonObject &arguments);

ToolDefinition makeProjectInspectTool();
ToolDefinition makeForumSearchTool();
ToolDefinition makeTimelineSummaryTool();
ToolRegistry createShotcutToolRegistry();

} // namespace Mcp
} // namespace Shotcut

#endif // SHOTCUT_MCP_SHOTCUTTOOLS_H
