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

#include "api/apidocs.h"

#include "mcp/mcpbridge.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QTextStream>

namespace Shotcut {
namespace Api {

namespace {

QString code(const QString &text)
{
    return QStringLiteral("<code>%1</code>").arg(text.toHtmlEscaped());
}

QString signature(const MemberInfo &member)
{
    QStringList args;
    for (const auto &param : member.params) {
        if (!param.isOption)
            args.append(param.name + (param.required ? QString() : QStringLiteral("?")));
    }
    if (member.takesOptions)
        args.append(member.optionsRequired ? QStringLiteral("options") : QStringLiteral("options?"));
    return QStringLiteral("shotcut.%1(%2)").arg(member.path, args.join(", "));
}

QString reference(const QList<ObjectInfo> &catalog)
{
    QString html;
    QTextStream out(&html);
    out << "<nav aria-label=\"API objects\"><ul>";
    for (const auto &object : catalog) {
        const QString name = object.path.isEmpty() ? QStringLiteral("shotcut") : object.path;
        out << "<li><a href=\"#" << name.toHtmlEscaped() << "\">" << code(name) << "</a></li>";
    }
    out << "</ul></nav>";
    for (const auto &object : catalog) {
        const QString name = object.path.isEmpty() ? QStringLiteral("shotcut") : object.path;
        out << "<section id=\"" << name.toHtmlEscaped() << "\"><h3>" << code(name) << "</h3><p>"
            << object.description.toHtmlEscaped() << "</p>";
        for (const auto &member : object.members) {
            out << "<section id=\"api-" << member.path.toHtmlEscaped() << "\"><h4>"
                << code(member.isProperty ? "shotcut." + member.path : signature(member))
                << "</h4><p>" << member.description.toHtmlEscaped() << "</p><p>"
                << (member.isProperty ? "Property type: " : "Return type: ")
                << code(member.type.isEmpty() ? "void" : member.type);
            if (member.isProperty) {
                out << ". Read-only; ";
                if (member.isObject)
                    out << "<a href=\"#" << member.path.toHtmlEscaped() << "\">API object</a>";
                else
                    out << "included in " << code("shotcut_state");
            } else {
                out << ". Kind: " << code(member.kind.isEmpty() ? "QML helper" : member.kind);
                if (member.kind == QLatin1String("read") || member.kind == QLatin1String("action"))
                    out << ". MCP tool: " << code(toolName(member.path));
                else if (member.kind == QLatin1String("edit"))
                    out << ". MCP: " << code("shotcut_edit") << " with " << code(member.path);
                else
                    out << ". Not exported through MCP";
            }
            out << ".</p>";
            if (!member.params.isEmpty()) {
                out << "<table><thead><tr><th>Argument</th><th>Type</th><th>Location</th>"
                       "<th>Required</th><th>Description</th></tr></thead><tbody>";
                for (const auto &param : member.params) {
                    const QString type = param.itemType.isEmpty()
                                             ? param.jsonType
                                             : param.jsonType + '<' + param.itemType + '>';
                    out << "<tr><td>" << code(param.name) << "</td><td>" << code(type)
                        << "</td><td>" << (param.isOption ? "options map" : "positional")
                        << "</td><td>" << (param.required ? "yes" : "no") << "</td><td>"
                        << param.description.toHtmlEscaped() << "</td></tr>";
                }
                out << "</tbody></table>";
            }
            out << "</section>";
        }
        out << "</section>";
    }
    return html;
}

bool writeFile(const QString &path, const QByteArray &data, QTextStream &errors)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        errors << "Cannot write " << path << ": " << file.errorString() << Qt::endl;
        return false;
    }
    return true;
}

} // namespace

QStringList documentationErrors(const QList<ObjectInfo> &catalog)
{
    QStringList errors;
    QSet<QString> paths;
    for (const auto &object : catalog) {
        const QString name = object.path.isEmpty() ? QStringLiteral("shotcut") : object.path;
        if (object.description.trimmed().isEmpty())
            errors.append(name + ": missing object description");
        for (const auto &member : object.members) {
            if (paths.contains(member.path))
                errors.append(member.path + ": duplicate API path");
            paths.insert(member.path);
            if (member.description.trimmed().isEmpty())
                errors.append(member.path + ": missing member description");
            if (member.type.isEmpty() && member.isProperty)
                errors.append(member.path + ": missing property type");
            if (!member.kind.isEmpty() && member.kind != QLatin1String("read")
                && member.kind != QLatin1String("edit") && member.kind != QLatin1String("action"))
                errors.append(member.path + ": invalid method kind");
            for (const auto &param : member.params) {
                const QString label = member.path + '.' + param.name;
                if (param.description.trimmed().isEmpty())
                    errors.append(label + ": missing argument description");
                if (param.jsonType.isEmpty())
                    errors.append(label + ": missing argument type");
            }
        }
    }
    return errors;
}

int runDocumentationCommand(const QStringList &arguments)
{
    QTextStream errors(stderr);
    const bool check = arguments.value(1) == QLatin1String("--check-api-docs");
    if ((check && arguments.size() != 2) || (!check && arguments.size() != 4)) {
        errors << "Usage: shotcut --check-api-docs\n"
                  "       shotcut --generate-api-docs <HTML template> <output directory>\n";
        return 1;
    }
    const auto catalog = apiCatalog();
    const auto missing = documentationErrors(catalog);
    if (!missing.isEmpty()) {
        errors << "API documentation is incomplete:\n" << missing.join('\n') << Qt::endl;
        return 1;
    }
    if (check)
        return 0;

    QString error;
    if (!generateDocumentation(arguments.at(2), arguments.at(3), error)) {
        errors << error;
        return 1;
    }
    return 0;
}

bool generateDocumentation(const QString &templatePath,
                           const QString &outputDirectory,
                           QString &error)
{
    error.clear();
    QTextStream errors(&error);
    const auto catalog = apiCatalog();
    const auto missing = documentationErrors(catalog);
    if (!missing.isEmpty()) {
        errors << "API documentation is incomplete:\n" << missing.join('\n') << Qt::endl;
        return false;
    }
    QFile source(templatePath);
    if (!source.open(QIODevice::ReadOnly)) {
        errors << "Cannot read documentation template: " << source.errorString() << Qt::endl;
        return false;
    }
    QString html = QString::fromUtf8(source.readAll());
    if (source.error() != QFileDevice::NoError) {
        errors << "Cannot read documentation template: " << source.errorString() << Qt::endl;
        return false;
    }
    if (html.count("%%API_REFERENCE%%") != 1 || html.count("%%MCP_REFERENCE%%") != 1) {
        errors << "Documentation template must contain each reference placeholder once.\n";
        return false;
    }

    QJsonArray objects;
    for (const auto &object : catalog)
        objects.append(QJsonObject::fromVariantMap(toVariant(object)));
    QJsonArray tools;
    QString toolHtml;
    QTextStream out(&toolHtml);
    for (const auto &tool : Mcp::makeApiTools()) {
        tools.append(QJsonObject{{"name", tool.name},
                                 {"description", tool.description},
                                 {"inputSchema", tool.parameters}});
        out << "<section id=\"mcp-" << tool.name.toHtmlEscaped() << "\"><h3>" << code(tool.name)
            << "</h3><p>" << tool.description.toHtmlEscaped()
            << "</p><details>"
               "<summary>Input schema</summary><pre>"
            << QString::fromUtf8(QJsonDocument(tool.parameters).toJson()).toHtmlEscaped()
            << "</pre></details></section>";
    }
    html.replace("%%API_REFERENCE%%", reference(catalog));
    html.replace("%%MCP_REFERENCE%%", toolHtml);
    const QDir directory(outputDirectory);
    if (!directory.mkpath(".")) {
        errors << "Cannot create documentation directory: " << directory.absolutePath() << Qt::endl;
        return false;
    }
    if (!writeFile(directory.filePath("index.html"), html.toUtf8(), errors)
        || !writeFile(directory.filePath("api.json"), QJsonDocument(objects).toJson(), errors)
        || !writeFile(directory.filePath("mcp-tools.json"), QJsonDocument(tools).toJson(), errors))
        return false;
    return true;
}

} // namespace Api
} // namespace Shotcut
