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

#include "addonmetadatahelpdialog.h"

#include "Logger.h"
#include "mltcontroller.h"

#include <MltProperties.h>
#include <QScopedPointer>
#include <QTextBrowser>
#include <QVBoxLayout>

static QString htmlEscapedMetadataText(const QString &text)
{
    QString escaped = text.toHtmlEscaped();
    escaped.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    escaped.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    escaped.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
    escaped.replace(QLatin1Char('\t'), QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;"));
    return escaped;
}

static bool isPurelyNumeric(const char *s)
{
    if (!s || !*s)
        return false;
    while (*s) {
        if (*s < '0' || *s > '9')
            return false;
        ++s;
    }
    return true;
}

static QString renderMetadataPropertiesHtml(Mlt::Properties &properties,
                                            int depth = 0,
                                            bool separateIndexed = false)
{
    if (!properties.is_valid())
        return QString();

    QString html = QStringLiteral("<table cellspacing='0' cellpadding='4' border='0' width='100%%' "
                                  "style='margin-left:%1px;'>")
                       .arg(depth * 16);

    for (int i = 0; i < properties.count(); ++i) {
        const char *rawName = properties.get_name(i);
        const bool isIndexed = isPurelyNumeric(rawName);
        const QString name = (rawName && *rawName) ? QString::fromUtf8(rawName) : QString();
        const char *rawValue = properties.get(i);
        const QString value = rawValue ? QString::fromUtf8(rawValue) : QString();
        Mlt::Properties child;
        if (rawName)
            child = Mlt::Properties(properties.get_data(rawName));
        const bool hasChild = child.is_valid() && child.count() > 0;

        // Detect a named key whose only content is a purely-indexed list (e.g. "parameters", "tags")
        bool isListField = false;
        if (!isIndexed && hasChild && value.isEmpty()) {
            isListField = true;
            for (int j = 0; j < child.count(); ++j) {
                if (!isPurelyNumeric(child.get_name(j))) {
                    isListField = false;
                    break;
                }
            }
        }

        // "parameters" entries get a separator rule; other lists (tags, values) do not
        const bool childSeparated = isListField && (name == QLatin1String("parameters"));

        if (isIndexed) {
            if (separateIndexed)
                html += QStringLiteral("<tr><td colspan='2' style='padding:0;'><hr/></td></tr>");
            html += QStringLiteral("<tr><td width='28%%'></td><td valign='top'>");
        } else if (isListField) {
            // Named list field: label on its own full-width row, list below
            html += QStringLiteral("<tr><td colspan='2' valign='top'><b>%1</b></td></tr>")
                        .arg(htmlEscapedMetadataText(name));
            html += QStringLiteral("<tr><td colspan='2' valign='top'>");
        } else {
            html += QStringLiteral(
                        "<tr><td valign='top' width='28%%'><b>%1</b></td><td valign='top'>")
                        .arg(htmlEscapedMetadataText(name));
        }

        if (!value.isEmpty())
            html += htmlEscapedMetadataText(value);
        else if (!hasChild && !isIndexed)
            html += QStringLiteral("<i>(empty)</i>");

        if (hasChild) {
            if (!value.isEmpty())
                html += QStringLiteral("<br/>");
            // List fields are already in a full-width cell; don't add extra indent depth
            html += renderMetadataPropertiesHtml(child,
                                                 isListField ? depth : depth + 1,
                                                 childSeparated);
        }

        html += QStringLiteral("</td></tr>");
    }

    html += QStringLiteral("</table>");
    return html;
}

static QString renderAddOnMetadataHtml(const QString &serviceName, Mlt::Properties &metadata)
{
    QString title = QString::fromUtf8(metadata.get("title")).trimmed();
    if (title.isEmpty())
        title = serviceName;

    QString html = QStringLiteral("<html><body>");
    html += QStringLiteral("<h2>%1</h2>").arg(htmlEscapedMetadataText(title));
    html += QStringLiteral("<p><b>%1:</b> %2</p>")
                .arg(QObject::tr("Service"))
                .arg(htmlEscapedMetadataText(serviceName));
    html += renderMetadataPropertiesHtml(metadata);
    html += QStringLiteral("</body></html>");
    return html;
}

AddOnMetadataHelpDialog *AddOnMetadataHelpDialog::create(const QString &serviceName, QWidget *parent)
{
    if (serviceName.isEmpty())
        return nullptr;

    QScopedPointer<Mlt::Properties> metadata(
        MLT.repository()->metadata(mlt_service_filter_type, serviceName.toUtf8().constData()));
    if (!metadata || !metadata->is_valid()) {
        LOG_WARNING() << "Failed to query add-on metadata for" << serviceName;
        return nullptr;
    }

    QString title = QString::fromUtf8(metadata->get("title")).trimmed();
    if (title.isEmpty())
        title = serviceName;

    return new AddOnMetadataHelpDialog(title,
                                       renderAddOnMetadataHtml(serviceName, *metadata),
                                       parent);
}

AddOnMetadataHelpDialog::AddOnMetadataHelpDialog(const QString &title,
                                                 const QString &html,
                                                 QWidget *parent)
    : QDialog(parent, Qt::Window)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(false);
    setWindowTitle(tr("Add-on Metadata: %1").arg(title));
    resize(560, 420);

    auto *layout = new QVBoxLayout(this);
    auto *view = new QTextBrowser(this);
    view->setReadOnly(true);
    view->setOpenExternalLinks(true);
    view->setHtml(html);
    layout->addWidget(view);
}