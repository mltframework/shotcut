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

#ifndef SHOTCUT_MCP_TOOLREGISTRY_H
#define SHOTCUT_MCP_TOOLREGISTRY_H

#include "mcp/tooldefinition.h"

#include <QJsonObject>
#include <QList>
#include <QMap>

namespace Shotcut {
namespace Mcp {

class ToolRegistry
{
public:
    ToolRegistry() = default;

    void registerTool(const ToolDefinition &tool);
    bool hasTool(const QString &name) const;
    ToolDefinition tool(const QString &name) const;
    QList<ToolDefinition> tools() const;

    // Executes the named tool and returns a JSON result object.
    // On failure, the error string is set and the result is empty.
    QJsonObject call(const QString &name,
                     const QJsonObject &arguments,
                     QString *error = nullptr) const;

private:
    QMap<QString, ToolDefinition> m_tools;
};

} // namespace Mcp
} // namespace Shotcut

#endif // SHOTCUT_MCP_TOOLREGISTRY_H
