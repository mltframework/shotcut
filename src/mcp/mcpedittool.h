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

#ifndef SHOTCUT_MCP_MCPEDITTOOL_H
#define SHOTCUT_MCP_MCPEDITTOOL_H

#include "mcp/tooldefinition.h"

namespace Shotcut {
namespace Mcp {

// shotcut_edit: runs a batch of API method calls as one undoable transaction.
// Any failure rolls back the whole batch. Each op may name its result with "as"
// so later ops can refer to it as "$name" or "$name.field".
ToolDefinition makeEditTool();

} // namespace Mcp
} // namespace Shotcut

#endif // SHOTCUT_MCP_MCPEDITTOOL_H
