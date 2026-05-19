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

#include "addonqmlgenerator.h"

#include <QDir>
#include <QFile>
#include <QStringConverter>
#include <QStringList>
#include <QTextStream>

static QString quotedJsString(const QString &value)
{
    QString escaped;
    escaped.reserve(value.size());

    for (const QChar &ch : value) {
        const ushort codepoint = ch.unicode();
        switch (codepoint) {
        case '\\':
            escaped += QStringLiteral("\\\\");
            break;
        case '\'':
            escaped += QStringLiteral("\\'");
            break;
        case '\n':
            escaped += QStringLiteral("\\n");
            break;
        case '\r':
            escaped += QStringLiteral("\\r");
            break;
        case '\t':
            escaped += QStringLiteral("\\t");
            break;
        case '\b':
            escaped += QStringLiteral("\\b");
            break;
        case '\f':
            escaped += QStringLiteral("\\f");
            break;
        case 0x2028:
            escaped += QStringLiteral("\\u2028");
            break;
        case 0x2029:
            escaped += QStringLiteral("\\u2029");
            break;
        default:
            if (codepoint < 0x20) {
                escaped += QStringLiteral("\\u%1").arg(static_cast<int>(codepoint),
                                                       4,
                                                       16,
                                                       QLatin1Char('0'));
            } else {
                escaped += ch;
            }
            break;
        }
    }

    return QStringLiteral("'%1'").arg(escaped);
}

static bool isGroupHeadingParameter(const AddOnParameterDescriptor &parameter)
{
    const QString parameterType = parameter.type.trimmed().toLower();
    const QString parameterName = parameter.name.trimmed().toLower();
    return parameterType == QStringLiteral("group")
           || (parameterName == QStringLiteral("group")
               && parameterType == QStringLiteral("string"));
}

bool AddOnQmlGenerator::generate(const AddOnFilterDescriptor &descriptor,
                                 const QDir &outputDir,
                                 const QString &qmlFileName,
                                 QString *errorMessage) const
{
    if (descriptor.service.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Missing add-on service identifier");
        return false;
    }

    QFile file(outputDir.filePath(qmlFileName));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to open generated add-on ui.qml for writing");
        return false;
    }

    QStringList quotedProperties;
    QStringList quotedTitles;
    QStringList quotedDefaults;
    QStringList quotedTypes;
    QStringList quotedUnits;
    QStringList quotedMinimums;
    QStringList quotedMaximums;
    QStringList quotedDescriptions;
    QStringList quotedValueLists;
    QStringList quotedKeyframeProperties;
    QStringList quotedKeyframeMapEntries;
    QStringList setControlsLines;
    int parameterIndex = 0;
    for (const auto &parameter : descriptor.parameters) {
        if (isGroupHeadingParameter(parameter))
            continue;

        quotedProperties << quotedJsString(parameter.name);
        if (!parameter.title.isEmpty()) {
            quotedTitles << QStringLiteral("%1: %2").arg(quotedJsString(parameter.name),
                                                         quotedJsString(parameter.title));
        }
        if (!parameter.defaultValue.isEmpty()) {
            quotedDefaults << QStringLiteral("%1: %2").arg(quotedJsString(parameter.name),
                                                           quotedJsString(parameter.defaultValue));
        }
        if (!parameter.type.isEmpty()) {
            quotedTypes << QStringLiteral("%1: %2").arg(quotedJsString(parameter.name),
                                                        quotedJsString(parameter.type));
        }
        if (!parameter.unit.isEmpty()) {
            quotedUnits << QStringLiteral("%1: %2").arg(quotedJsString(parameter.name),
                                                        quotedJsString(parameter.unit));
        }
        if (!parameter.minimum.isEmpty()) {
            quotedMinimums << QStringLiteral("%1: %2").arg(quotedJsString(parameter.name),
                                                           quotedJsString(parameter.minimum));
        }
        if (!parameter.maximum.isEmpty()) {
            quotedMaximums << QStringLiteral("%1: %2").arg(quotedJsString(parameter.name),
                                                           quotedJsString(parameter.maximum));
        }
        if (!parameter.description.isEmpty()) {
            quotedDescriptions << QStringLiteral("%1: %2").arg(quotedJsString(parameter.name),
                                                               quotedJsString(
                                                                   parameter.description));
        }
        if (!parameter.values.isEmpty()) {
            QStringList quotedOptions;
            for (const auto &value : parameter.values)
                quotedOptions << quotedJsString(value);
            quotedValueLists << QStringLiteral("%1: [%2]")
                                    .arg(quotedJsString(parameter.name),
                                         quotedOptions.join(QStringLiteral(", ")));
        }

        const QString parameterType = parameter.type.trimmed().toLower();
        const bool supportsGeneratedKeyframes = !parameter.isReadOnly && parameter.supportsKeyframes
                                                && (parameterType == QStringLiteral("integer")
                                                    || parameterType == QStringLiteral("float")
                                                    || parameterType == QStringLiteral("color"));
        const QString parameterId = QStringLiteral("param_%1").arg(parameterIndex++);
        const QString editorId = parameterId + QStringLiteral("_editor");
        const QString keyframesId = parameterId + QStringLiteral("_keyframes");
        const QString nameLiteral = quotedJsString(parameter.name);
        if (supportsGeneratedKeyframes) {
            quotedKeyframeProperties << nameLiteral;
            quotedKeyframeMapEntries << QStringLiteral("%1: true").arg(nameLiteral);
        }
        if (parameter.isReadOnly) {
            setControlsLines << QStringLiteral("        %1.text = root.textValue(%2);")
                                    .arg(editorId, nameLiteral);
        } else if (parameterType == QStringLiteral("string") && !parameter.values.isEmpty()) {
            setControlsLines << QStringLiteral(
                                    "        var %1_options = root.propertyValues[%2] || [];")
                                    .arg(editorId, nameLiteral);
            setControlsLines << QStringLiteral("        var %1_value = String(root.textValue(%2));")
                                    .arg(editorId, nameLiteral);
            setControlsLines << QStringLiteral(
                                    "        var %1_index = %1_options.indexOf(%1_value);")
                                    .arg(editorId);
            setControlsLines << QStringLiteral(
                                    "        %1.currentIndex = %1_index >= 0 ? %1_index : 0;")
                                    .arg(editorId);
        } else if (parameterType == QStringLiteral("color")) {
            setControlsLines << QStringLiteral("        %1.value = root.colorValue(%2);")
                                    .arg(editorId, nameLiteral);
        } else if (parameterType == QStringLiteral("boolean")) {
            setControlsLines << QStringLiteral("        %1.checked = root.booleanValue(%2);")
                                    .arg(editorId, nameLiteral);
        } else if (parameterType == QStringLiteral("integer")
                   || parameterType == QStringLiteral("float")) {
            setControlsLines
                << QStringLiteral(
                       "        %1.value = root.numericValue(%2, root.propertyType(%2));")
                       .arg(editorId, nameLiteral);
        } else {
            setControlsLines << QStringLiteral("        %1.text = root.textValue(%2);")
                                    .arg(editorId, nameLiteral);
        }
        if (supportsGeneratedKeyframes) {
            setControlsLines << QStringLiteral(
                                    "        %1.checked = filter.animateIn <= 0 && "
                                    "filter.animateOut <= 0 && filter.keyframeCount(%2) > 0;")
                                    .arg(keyframesId, nameLiteral);
        }
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream
        << "import QtQuick\n"
           "import QtQuick.Controls\n"
           "import QtQuick.Layouts\n\n"
           "import Shotcut.Controls as Shotcut\n\n"
           "Shotcut.KeyframableFilter {\n"
           "    id: root\n"
           "    signal metadataHelpRequested(string service)\n"
           "    property var propertyNames: ["
        << quotedProperties.join(QStringLiteral(", "))
        << "]\n\n"
           "    property var propertyTitles: {"
        << quotedTitles.join(QStringLiteral(", "))
        << "}\n\n"
           "    property var propertyDefaults: {"
        << quotedDefaults.join(QStringLiteral(", "))
        << "}\n\n"
           "    property var propertyTypes: {"
        << quotedTypes.join(QStringLiteral(", "))
        << "}\n\n"
           "    property var propertyUnits: {"
        << quotedUnits.join(QStringLiteral(", "))
        << "}\n\n"
           "    property var propertyMinimums: {"
        << quotedMinimums.join(QStringLiteral(", "))
        << "}\n\n"
           "    property var propertyMaximums: {"
        << quotedMaximums.join(QStringLiteral(", "))
        << "}\n\n"
           "    property var propertyDescriptions: {"
        << quotedDescriptions.join(QStringLiteral(", "))
        << "}\n\n"
           "    property var propertyValues: {"
        << quotedValueLists.join(QStringLiteral(", "))
        << "}\n\n"
           "    keyframableParameters: ["
        << quotedKeyframeProperties.join(QStringLiteral(", "))
        << "]\n"
           "    startValues: []\n"
           "    middleValues: []\n"
           "    endValues: []\n\n"
           "    property var propertyKeyframes: {"
        << quotedKeyframeMapEntries.join(QStringLiteral(", "))
        << "}\n\n"
           "    property string filterService: "
        << quotedJsString(descriptor.service)
        << "\n\n"
           "    property string filterDescription: "
        << quotedJsString(descriptor.description)
        << "\n\n"
           "    function isKeyframableProperty(name) {\n"
           "        return !!(root.propertyKeyframes && root.propertyKeyframes[name]);\n"
           "    }\n\n"
           "    function propertyType(name) {\n"
           "        if (!root.propertyTypes)\n"
           "            return '';\n"
           "        var type = root.propertyTypes[name];\n"
           "        return (type !== undefined && type !== null) ? String(type).toLowerCase() : "
           "'';\n"
           "    }\n\n"
           "    function isNumericType(type) {\n"
           "        return type === 'integer' || type === 'float';\n"
           "    }\n\n"
           "    function isIntegerType(type) {\n"
           "        return type === 'integer';\n"
           "    }\n\n"
           "    function numericSuffix(name) {\n"
           "        if (!root.propertyUnits)\n"
           "            return '';\n"
           "        var unit = root.propertyUnits[name];\n"
           "        if (unit === undefined || unit === null || unit === '')\n"
           "            return '';\n"
           "        unit = String(unit);\n"
           "        return unit.startsWith(' ') ? unit : (' ' + unit);\n"
           "    }\n\n"
           "    function isBooleanType(type) {\n"
           "        return type === 'boolean';\n"
           "    }\n\n"
           "    function isColorType(type) {\n"
           "        return type === 'color';\n"
           "    }\n\n"
           "    function colorValue(name) {\n"
           "        var value = filter.get(name);\n"
           "        if (value === undefined || value === null || value === '')\n"
           "            value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;\n"
           "        return (value !== undefined && value !== null && value !== '') ? value : "
           "'#ffffffff';\n"
           "    }\n\n"
           "    function defaultColorValue(name) {\n"
           "        var value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;\n"
           "        return (value !== undefined && value !== null && value !== '') ? value : "
           "'#ffffffff';\n"
           "    }\n\n"
           "    function booleanValue(name) {\n"
           "        var value = filter.get(name);\n"
           "        if (value === undefined || value === null || value === '')\n"
           "            value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;\n"
           "        if (value === undefined || value === null)\n"
           "            return false;\n"
           "        if (value === true || value === 1)\n"
           "            return true;\n"
           "        var text = String(value).toLowerCase();\n"
           "        return text === '1' || text === 'true';\n"
           "    }\n\n"
           "    function defaultBooleanValue(name) {\n"
           "        var value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;\n"
           "        if (value === undefined || value === null)\n"
           "            return false;\n"
           "        if (value === true || value === 1)\n"
           "            return true;\n"
           "        var text = String(value).toLowerCase();\n"
           "        return text === '1' || text === 'true';\n"
           "    }\n\n"
           "    function textValue(name) {\n"
           "        var value = filter.get(name);\n"
           "        if (value !== undefined && value !== null && value !== '')\n"
           "            return value;\n"
           "        var d = root.propertyDefaults ? root.propertyDefaults[name] : undefined;\n"
           "        return (d !== undefined && d !== null) ? d : '';\n"
           "    }\n\n"
           "    function defaultTextValue(name) {\n"
           "        var value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;\n"
           "        return (value !== undefined && value !== null) ? value : '';\n"
           "    }\n\n"
           "    function numericBound(name, useMin) {\n"
           "        var map = useMin ? root.propertyMinimums : root.propertyMaximums;\n"
           "        if (!map)\n"
           "            return useMin ? 0 : 100;\n"
           "        var raw = map[name];\n"
           "        if (raw === undefined || raw === null || raw === '')\n"
           "            return useMin ? 0 : 100;\n"
           "        var parsed = Number(raw);\n"
           "        if (isNaN(parsed))\n"
           "            return useMin ? 0 : 100;\n"
           "        return parsed;\n"
           "    }\n\n"
           "    function numericValue(name, type) {\n"
           "        var value;\n"
           "        if (root.isKeyframableProperty(name))\n"
           "            value = filter.getDouble(name, root.getPosition());\n"
           "        else\n"
           "            value = filter.get(name);\n"
           "        if (value === undefined || value === null || value === '')\n"
           "            value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;\n"
           "        var parsed = Number(value);\n"
           "        if (isNaN(parsed)) {\n"
           "            parsed = numericBound(name, true);\n"
           "            var maxBound = numericBound(name, false);\n"
           "            if (parsed > maxBound)\n"
           "                parsed = maxBound;\n"
           "        }\n"
           "        if (isIntegerType(type))\n"
           "            parsed = Math.round(parsed);\n"
           "        return parsed;\n"
           "    }\n\n"
           "    function defaultNumericValue(name, type) {\n"
           "        var value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;\n"
           "        var parsed = Number(value);\n"
           "        if (isNaN(parsed)) {\n"
           "            parsed = numericBound(name, true);\n"
           "            var maxBound = numericBound(name, false);\n"
           "            if (parsed > maxBound)\n"
           "                parsed = maxBound;\n"
           "        }\n"
           "        if (isIntegerType(type))\n"
           "            parsed = Math.round(parsed);\n"
           "        return parsed;\n"
           "    }\n\n"
           "    function setControls() {\n"
           "        blockUpdate = true;\n";

    if (!setControlsLines.isEmpty()) {
        stream << setControlsLines.join(QStringLiteral("\n")) << "\n";
    }

    stream
        << "        blockUpdate = false;\n"
           "    }\n\n"
           "    width: 360\n"
           "    height: gridLayout.implicitHeight + 16\n\n"
           "    Component.onCompleted: {\n"
           "        if (filter.isNew) {\n"
           "            for (var i = 0; i < propertyNames.length; ++i) {\n"
           "                var p = propertyNames[i];\n"
           "                var d = root.propertyDefaults[p];\n"
           "                var type = propertyType(p);\n"
           "                if (d !== undefined && d !== null && d !== '')\n"
           "                    filter.set(p, isBooleanType(type) ? (booleanValue(p) ? '1' : '0') "
           ": (isNumericType(type) ? Number(d) : d));\n"
           "            }\n"
           "            filter.savePreset(propertyNames);\n"
           "        }\n"
           "        setControls();\n"
           "        if (keyframableParameters.length > 0)\n"
           "            initializeSimpleKeyframes();\n"
           "    }\n\n"
           "    GridLayout {\n"
           "        id: gridLayout\n"
           "        anchors.left: parent.left\n"
           "        anchors.right: parent.right\n"
           "        anchors.top: parent.top\n"
           "        anchors.margins: 8\n"
           "        columns: 4\n"
           "        columnSpacing: 8\n"
           "        rowSpacing: 6\n\n"
           "        Label {\n"
           "            Layout.columnSpan: 3\n"
           "            Layout.fillWidth: true\n"
           "            text: root.filterDescription.length > 0 ? qsTr('Add-on Filter: "
           "%1').arg(root.filterDescription) : qsTr('Add-on Filter')\n"
           "            wrapMode: Text.Wrap\n"
           "        }\n\n"
           "        ToolButton {\n"
           "            icon.name: 'help-contextual'\n"
           "            icon.source: 'qrc:///icons/oxygen/32x32/actions/help-contextual.png'\n"
           "            text: ''\n"
           "            display: AbstractButton.IconOnly\n"
           "            ToolTip.visible: hovered\n"
           "            ToolTip.text: qsTr('Help')\n"
           "            Layout.alignment: Qt.AlignRight | Qt.AlignTop\n"
           "            onClicked: root.metadataHelpRequested(root.filterService)\n"
           "        }\n\n"
           "        Label {\n"
           "            text: qsTr('Preset')\n"
           "            horizontalAlignment: Text.AlignRight\n"
           "            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter\n"
           "        }\n"
           "\n"
           "        Shotcut.Preset {\n"
           "            id: preset\n"
           "            Layout.columnSpan: 3\n"
           "            Layout.fillWidth: true\n"
           "            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter\n"
           "            parameters: root.propertyNames ? root.propertyNames.slice(0) : []\n"
           "            onBeforePresetLoaded: {\n"
           "                filter.resetProperty('shotcut:animIn');\n"
           "                filter.resetProperty('shotcut:animOut');\n"
           "                if (keyframableParameters.length > 0)\n"
           "                    resetSimpleKeyframes();\n"
           "                for (var i = 0; i < root.propertyNames.length; ++i)\n"
           "                    filter.resetProperty(root.propertyNames[i]);\n"
           "            }\n"
           "            onPresetSelected: {\n"
           "                filter.animateIn = Math.round(filter.getDouble('shotcut:animIn'));\n"
           "                filter.animateOut = Math.round(filter.getDouble('shotcut:animOut'));\n"
           "                root.setControls();\n"
           "                if (keyframableParameters.length > 0)\n"
           "                    initializeSimpleKeyframes();\n"
           "            }\n"
           "        }\n\n"
           "";

    int rowIndex = 0;
    for (const auto &parameter : descriptor.parameters) {
        if (isGroupHeadingParameter(parameter)) {
            QString sectionTitle = parameter.title;
            if (sectionTitle.isEmpty())
                sectionTitle = parameter.defaultValue;
            if (sectionTitle.isEmpty())
                sectionTitle = parameter.name;

            stream << "        Label {\n"
                      "            Layout.columnSpan: 4\n"
                      "            Layout.fillWidth: true\n"
                      "            horizontalAlignment: Text.AlignHCenter\n"
                      "            font.bold: true\n"
                      "            text: "
                   << quotedJsString(sectionTitle)
                   << "\n"
                      "        }\n\n";
            continue;
        }

        const QString parameterType = parameter.type.trimmed().toLower();
        const bool supportsGeneratedKeyframes = !parameter.isReadOnly && parameter.supportsKeyframes
                                                && (parameterType == QStringLiteral("integer")
                                                    || parameterType == QStringLiteral("float")
                                                    || parameterType == QStringLiteral("color"));
        const QString parameterId = QStringLiteral("param_%1").arg(rowIndex++);
        const QString hoverId = parameterId + QStringLiteral("_hover");
        const QString editorId = parameterId + QStringLiteral("_editor");
        const QString keyframesId = parameterId + QStringLiteral("_keyframes");

        const QString nameLiteral = quotedJsString(parameter.name);

        if (parameterType == QStringLiteral("boolean") && !parameter.isReadOnly) {
            stream << "        Item { }\n\n";
        } else {
            stream << "        Label {\n"
                      "            text: (root.propertyTitles && root.propertyTitles["
                   << nameLiteral << "]) ? root.propertyTitles[" << nameLiteral
                   << "] : " << nameLiteral
                   << "\n"
                      "            horizontalAlignment: Text.AlignRight\n"
                      "            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter\n"
                      "            elide: Text.ElideRight\n"
                      "            ToolTip.delay: 400\n"
                      "            ToolTip.visible: "
                   << hoverId
                   << ".containsMouse\n"
                      ""
                      "            ToolTip.text: {\n"
                      "                if (root.propertyDescriptions && root.propertyDescriptions["
                   << nameLiteral
                   << "])\n"
                      "                    return root.propertyDescriptions["
                   << nameLiteral
                   << "];\n"
                      "                return text;\n"
                      "            }\n\n"
                      "            MouseArea {\n"
                      "                id: "
                   << hoverId
                   << "\n"
                      ""
                      "                anchors.fill: parent\n"
                      "                hoverEnabled: true\n"
                      "                acceptedButtons: Qt.NoButton\n"
                      "            }\n"
                      "        }\n\n";
        }

        if (parameter.isReadOnly) {
            stream << "        TextField {\n"
                      "            id: "
                   << editorId
                   << "\n"
                      ""
                      "            Layout.columnSpan: 3\n"
                      "            Layout.fillWidth: true\n"
                      "            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter\n"
                      "            readonly property string propertyName: "
                   << nameLiteral
                   << "\n"
                      "            text: root.textValue(propertyName)\n"
                      "            selectByMouse: true\n"
                      "            readOnly: true\n"
                      "        }\n";
        } else if (parameterType == QStringLiteral("string") && !parameter.values.isEmpty()) {
            stream << "        ComboBox {\n"
                      "            id: "
                   << editorId
                   << "\n"
                      ""
                      "            Layout.fillWidth: true\n"
                      "            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter\n"
                      "            readonly property string propertyName: "
                   << nameLiteral
                   << "\n"
                      "            model: root.propertyValues[propertyName] || []\n"
                      "            onActivated: {\n"
                      "                var option = model[currentIndex];\n"
                      "                var current = filter.get(propertyName);\n"
                      "                if (String(current) !== String(option))\n"
                      "                    filter.set(propertyName, option);\n"
                      "            }\n"
                      "        }\n";
        } else if (parameterType == QStringLiteral("color")) {
            stream << "        Shotcut.ColorPicker {\n"
                      "                id: "
                   << editorId
                   << "\n"
                      ""
                      "            Layout.fillWidth: true\n"
                      "            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter\n"
                      "            readonly property string propertyName: "
                   << nameLiteral
                   << "\n"
                      "            readonly property string typeName: "
                      "root.propertyType(propertyName)\n"
                      "            property bool isReady: false\n"
                      "            value: root.colorValue(propertyName)\n"
                      "            alpha: true\n"
                      "            Component.onCompleted: isReady = true\n"
                      "            onValueChanged: {\n"
                      "                if (!isReady)\n"
                      "                    return;\n"
                      "                if (root.isKeyframableProperty(propertyName)) {\n"
                      "                    root.updateFilter(propertyName, Qt.color(value), "
                   << keyframesId
                   << ", root.getPosition());\n"
                      "                } else {\n"
                      "                    var current = filter.get(propertyName);\n"
                      "                    if (String(current) !== String(value))\n"
                      "                        filter.set(propertyName, value);\n"
                      "                }\n"
                      "            }\n"
                      "        }\n";
        } else if (parameterType == QStringLiteral("boolean")) {
            stream
                << "        CheckBox {\n"
                   "                id: "
                << editorId
                << "\n"
                   ""
                   "            Layout.fillWidth: false\n"
                   "            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter\n"
                   "            readonly property string propertyName: "
                << nameLiteral
                << "\n"
                   "            readonly property string typeName: "
                   "root.propertyType(propertyName)\n"
                   "            text: (root.propertyTitles && root.propertyTitles[propertyName]) ? "
                   "root.propertyTitles[propertyName] : propertyName\n"
                   "            ToolTip.delay: 400\n"
                   "            ToolTip.visible: hovered\n"
                   "            ToolTip.text: {\n"
                   "                if (root.propertyDescriptions && "
                   "root.propertyDescriptions[propertyName])\n"
                   "                    return root.propertyDescriptions[propertyName];\n"
                   "                return text;\n"
                   "            }\n"
                   "            checked: root.booleanValue(propertyName)\n"
                   "            onToggled: {\n"
                   "                var nextValue = checked ? '1' : '0';\n"
                   "                if (root.isKeyframableProperty(propertyName)) {\n"
                   "                    root.updateFilter(propertyName, Number(nextValue), "
                << keyframesId
                << ", root.getPosition());\n"
                   "                } else {\n"
                   "                    var current = filter.get(propertyName);\n"
                   "                    if (String(current) !== nextValue)\n"
                   "                        filter.set(propertyName, nextValue);\n"
                   "                }\n"
                   "            }\n"
                   "        }\n";
        } else if (parameterType == QStringLiteral("integer")
                   || parameterType == QStringLiteral("float")) {
            stream << "        Shotcut.SliderSpinner {\n"
                      "            id: "
                   << editorId
                   << "\n"
                      ""
                      "            Layout.fillWidth: true\n"
                      "            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter\n"
                      "            Layout.minimumWidth: 180\n"
                      "            readonly property string propertyName: "
                   << nameLiteral
                   << "\n"
                      "            readonly property string typeName: "
                      "root.propertyType(propertyName)\n"
                      "            value: root.numericValue(propertyName, typeName)\n"
                      "            minimumValue: Math.min(root.numericBound(propertyName, true), "
                      "root.numericBound(propertyName, false))\n"
                      "            maximumValue: Math.max(root.numericBound(propertyName, true), "
                      "root.numericBound(propertyName, false))\n"
                      "            decimals: root.isIntegerType(typeName) ? 0 : 3\n"
                      "            suffix: root.numericSuffix(propertyName)\n"
                      "            onValueChanged: {\n"
                      "                if (root.isKeyframableProperty(propertyName)) {\n"
                      "                    root.updateFilter(propertyName, value, "
                   << keyframesId
                   << ", root.getPosition());\n"
                      "                } else {\n"
                      "                    var current = Number(filter.get(propertyName));\n"
                      "                    if (isNaN(current) || current !== value)\n"
                      "                        filter.set(propertyName, value);\n"
                      "                }\n"
                      "            }\n"
                      "        }\n";
        } else {
            stream << "        TextField {\n"
                      "            id: "
                   << editorId
                   << "\n"
                      ""
                      "            Layout.fillWidth: true\n"
                      "            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter\n"
                      "            selectByMouse: true\n"
                      "            readonly property string propertyName: "
                   << nameLiteral
                   << "\n"
                      "            readonly property string typeName: "
                      "root.propertyType(propertyName)\n"
                      "            text: {\n"
                      "                var value = filter.get(propertyName);\n"
                      "                if (value !== undefined && value !== null && value !== '')\n"
                      "                    return value;\n"
                      "                var d = root.propertyDefaults[propertyName];\n"
                      "                return (d !== undefined && d !== null) ? d : '';\n"
                      "            }\n"
                      "            onEditingFinished: {\n"
                      "                if (root.isKeyframableProperty(propertyName)) {\n"
                      "                    root.updateFilter(propertyName, text, "
                   << keyframesId
                   << ", root.getPosition());\n"
                      "                } else {\n"
                      "                    var current = filter.get(propertyName);\n"
                      "                    if (String(current) !== text)\n"
                      "                        filter.set(propertyName, text);\n"
                      "                }\n"
                      "            }\n"
                      "        }\n";
        }

        if (!parameter.isReadOnly) {
            stream << "\n"
                      "        Shotcut.UndoButton {\n"
                      "            readonly property string propertyName: "
                   << nameLiteral << "\n";

            if (!supportsGeneratedKeyframes) {
                stream << "            Layout.columnSpan: 2\n";
            }

            stream << "            readonly property string typeName: "
                      "root.propertyType(propertyName)\n"
                      "            onClicked: {\n";

            if (parameterType == QStringLiteral("color")) {
                stream
                    << "                var defaultValue = root.defaultColorValue(propertyName);\n"
                       "                if (root.isKeyframableProperty(propertyName))\n"
                       "                    root.updateFilter(propertyName, "
                       "Qt.color(defaultValue), "
                    << keyframesId
                    << ", root.getPosition());\n"
                       "                else\n"
                       "                    filter.set(propertyName, defaultValue);\n"
                       "                root.setControls();\n";
            } else if (parameterType == QStringLiteral("boolean")) {
                stream
                    << "                var defaultValue = "
                       "root.defaultBooleanValue(propertyName);\n"
                       "                if (root.isKeyframableProperty(propertyName))\n"
                       "                    root.updateFilter(propertyName, defaultValue ? 1 : 0, "
                    << keyframesId
                    << ", root.getPosition());\n"
                       "                else\n"
                       "                    filter.set(propertyName, defaultValue ? '1' : '0');\n"
                       "                root.setControls();\n";
            } else if (parameterType == QStringLiteral("integer")
                       || parameterType == QStringLiteral("float")) {
                stream
                    << "                var defaultValue = root.defaultNumericValue(propertyName, "
                       "typeName);\n"
                       "                if (root.isKeyframableProperty(propertyName))\n"
                       "                    root.updateFilter(propertyName, defaultValue, "
                    << keyframesId
                    << ", root.getPosition());\n"
                       "                else\n"
                       "                    filter.set(propertyName, defaultValue);\n"
                       "                root.setControls();\n";
            } else {
                stream
                    << "                var defaultValue = root.defaultTextValue(propertyName);\n"
                       "                if (root.isKeyframableProperty(propertyName))\n"
                       "                    root.updateFilter(propertyName, defaultValue, "
                    << keyframesId
                    << ", root.getPosition());\n"
                       "                else\n"
                       "                    filter.set(propertyName, defaultValue);\n"
                       "                root.setControls();\n";
            }

            stream << "            }\n"
                      "        }\n";

            if (supportsGeneratedKeyframes) {
                stream << "\n"
                          "        Shotcut.KeyframesButton {\n"
                          "            id: "
                       << keyframesId
                       << "\n"
                          "            readonly property string propertyName: "
                       << nameLiteral
                       << "\n"
                          "            visible: root.isKeyframableProperty(propertyName)\n"
                          "            Layout.preferredWidth: visible ? implicitWidth : 0\n"
                          "            onToggled: {\n";

                if (parameterType == QStringLiteral("color")) {
                    stream << "                root.toggleKeyframes(checked, propertyName, "
                              "Qt.color("
                           << editorId << ".value));\n";
                } else if (parameterType == QStringLiteral("boolean")) {
                    stream << "                root.toggleKeyframes(checked, propertyName, "
                           << editorId << ".checked ? 1 : 0);\n";
                } else if (parameterType == QStringLiteral("integer")
                           || parameterType == QStringLiteral("float")) {
                    stream << "                root.toggleKeyframes(checked, propertyName, "
                           << editorId << ".value);\n";
                } else {
                    stream << "                root.toggleKeyframes(checked, propertyName, "
                           << editorId << ".text);\n";
                }

                stream << "            }\n"
                          "        }\n\n";
            } else {
                stream << "\n";
            }
        }
    }

    stream << "        Label {\n"
              "            Layout.columnSpan: 4\n"
              "            Layout.fillWidth: true\n"
              "            visible: propertyNames.length === 0\n"
              "            text: qsTr('No properties were discovered for this filter service.')\n"
              "            wrapMode: Text.Wrap\n"
              "        }\n"
              "    }\n"
              "\n"
              "    Connections {\n"
              "        target: filter\n"
              "\n"
              "        function onAnimateInChanged() {\n"
              "            root.setControls();\n"
              "        }\n"
              "\n"
              "        function onAnimateOutChanged() {\n"
              "            root.setControls();\n"
              "        }\n"
              "    }\n"
              "\n"
              "    Connections {\n"
              "        target: producer\n"
              "\n"
              "        function onPositionChanged() {\n"
              "            root.setControls();\n"
              "        }\n"
              "    }\n"
              "\n"
              "}\n";

    file.close();
    if (errorMessage)
        errorMessage->clear();
    return true;
}
