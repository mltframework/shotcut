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

#ifndef SHOTCUT_MCP_MCPBRIDGE_H
#define SHOTCUT_MCP_MCPBRIDGE_H

#include "mcp/tooldefinition.h"

#include <QJsonObject>
#include <QList>

namespace Shotcut {
namespace Mcp {

// MCP tools generated from the scripting API: shotcut_state, shotcut_capabilities,
// shotcut_edit, and one tool per method marked with Q_CLASSINFO("mcp.<method>", "read" or "action").
QList<ToolDefinition> makeApiTools();

// Replaces a response that would overflow a model's context with a result_too_large error.
QJsonObject limitSize(const QJsonObject &response);

} // namespace Mcp
} // namespace Shotcut

#endif // SHOTCUT_MCP_MCPBRIDGE_H
