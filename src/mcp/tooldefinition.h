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

#ifndef SHOTCUT_MCP_TOOLDEFINITION_H
#define SHOTCUT_MCP_TOOLDEFINITION_H

#include <QJsonObject>
#include <QString>

#include <functional>

namespace Shotcut {
namespace Mcp {

struct ToolDefinition
{
    QString name;
    QString description;
    QJsonObject parameters;
    using Handler = std::function<QJsonObject(const QJsonObject &)>;
    Handler handler;
};

} // namespace Mcp
} // namespace Shotcut

#endif // SHOTCUT_MCP_TOOLDEFINITION_H
