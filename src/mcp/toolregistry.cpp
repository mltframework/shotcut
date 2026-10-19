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

#include "mcp/toolregistry.h"

#include <QJsonValue>

namespace Shotcut {
namespace Mcp {

void ToolRegistry::registerTool(const ToolDefinition &tool)
{
    if (tool.name.isEmpty())
        return;

    m_tools.insert(tool.name, tool);
}

bool ToolRegistry::hasTool(const QString &name) const
{
    return m_tools.contains(name);
}

ToolDefinition ToolRegistry::tool(const QString &name) const
{
    return m_tools.value(name, ToolDefinition());
}

QList<ToolDefinition> ToolRegistry::tools() const
{
    return m_tools.values();
}

QJsonObject ToolRegistry::call(const QString &name,
                               const QJsonObject &arguments,
                               QString *error) const
{
    if (!m_tools.contains(name)) {
        if (error)
            *error = QStringLiteral("Unknown tool: %1").arg(name);
        return QJsonObject();
    }

    const ToolDefinition tool = m_tools.value(name);
    if (!tool.handler) {
        if (error)
            *error = QStringLiteral("Tool %1 has no handler.").arg(name);
        return QJsonObject();
    }

    try {
        return tool.handler(arguments);
    } catch (...) {
        if (error)
            *error = QStringLiteral("Tool %1 threw an exception.").arg(name);
        return QJsonObject();
    }
}

} // namespace Mcp
} // namespace Shotcut
