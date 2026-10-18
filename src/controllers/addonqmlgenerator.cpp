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

static bool hasDividerLayoutHint(const AddOnParameterDescriptor &parameter)
{
    return parameter.layoutHint.trimmed().toLower() == QStringLiteral("divider");
}

static bool isChannelMaskParameter(const AddOnFilterDescriptor &descriptor,
                                   const AddOnParameterDescriptor &parameter)
{
    return (descriptor.service.startsWith(QStringLiteral("ladspa."), Qt::CaseSensitive)
            || descriptor.service.startsWith(QStringLiteral("vst2."), Qt::CaseSensitive))
           && parameter.name.trimmed().toLower() == QStringLiteral("channel_mask");
}

static bool isPointOrSizeWidget(const AddOnParameterDescriptor &parameter)
{
    const QString widget = parameter.widget.trimmed().toLower();
    return widget == QStringLiteral("point") || widget == QStringLiteral("size");
}

static bool isRectangleVuiParameter(const AddOnFilterDescriptor &descriptor,
                                    const AddOnParameterDescriptor &parameter)
{
    if (descriptor.isAudio || parameter.isReadOnly)
        return false;
    return parameter.type.trimmed().toLower() == QStringLiteral("rect")
           && !isPointOrSizeWidget(parameter);
}

static const AddOnParameterDescriptor *rectangleVuiParameter(const AddOnFilterDescriptor &descriptor)
{
    for (const auto &parameter : descriptor.parameters) {
        if (isRectangleVuiParameter(descriptor, parameter))
            return &parameter;
    }
    return nullptr;
}

static bool isRotationParameter(const AddOnParameterDescriptor &parameter)
{
    if (parameter.isReadOnly)
        return false;
    const QString type = parameter.type.trimmed().toLower();
    if (type != QStringLiteral("integer") && type != QStringLiteral("float"))
        return false;
    const QString name = parameter.name.trimmed().toLower();
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    const QString leaf = dot >= 0 ? name.mid(dot + 1) : name;
    return leaf == QStringLiteral("rotate") || leaf == QStringLiteral("rotation");
}

static const AddOnParameterDescriptor *rotationVuiParameter(const AddOnFilterDescriptor &descriptor)
{
    if (!rectangleVuiParameter(descriptor))
        return nullptr;
    for (const auto &parameter : descriptor.parameters) {
        if (isRotationParameter(parameter))
            return &parameter;
    }
    return nullptr;
}

static QString rectanglePresetSeedJs(const AddOnFilterDescriptor &descriptor)
{
    QString seed;
    const AddOnParameterDescriptor *rectParameter = rectangleVuiParameter(descriptor);
    if (rectParameter && rectParameter->supportsKeyframes) {
        const QString name = quotedJsString(rectParameter->name);
        seed += QStringLiteral("                (function() {\n"
                               "                    var _name = ");
        seed += name;
        seed += QStringLiteral(
            ";\n"
            "                    function _px(position) {\n"
            "                        var _value = filter.getRect(_name, position);\n"
            "                        var _raw = filter.get(_name);\n"
            "                        var _text = (_raw === undefined || _raw === null) ? '' : "
            "String(_raw);\n"
            "                        var _flag = filter.get('shotcut:rectPixels');\n"
            "                        if (root.propertyNormalizedCoordinates &&\n"
            "                                root.propertyNormalizedCoordinates[_name] === 'yes' "
            "&&\n"
            "                                _text.indexOf('%') < 0 && _flag != 1 && _flag != '1' "
            "&&\n"
            "                                root.rectangleIsUnitless(_value)) {\n"
            "                            _value = Qt.rect(_value.x * profile.width,\n"
            "                                _value.y * profile.height,\n"
            "                                _value.width * profile.width,\n"
            "                                _value.height * profile.height);\n"
            "                        }\n"
            "                        return _value;\n"
            "                    }\n"
            "                    var _middle = filter.animateIn > 0 ? "
            "Math.max(filter.animateIn - 1, 0) : 0;\n"
            "                    filter.set('_shotcut:startValue', _px(0));\n"
            "                    filter.set('_shotcut:middleValue', _px(_middle));\n"
            "                    filter.set('_shotcut:endValue', "
            "_px(Math.max(filter.duration - 1, 0)));\n"
            "                })();\n");
    }

    return seed;
}

static bool writeRectangleVui(const AddOnFilterDescriptor &descriptor,
                              const QDir &outputDir,
                              QString *errorMessage)
{
    const QString vuiPath = outputDir.filePath(QStringLiteral("vui.qml"));
    const AddOnParameterDescriptor *rectParameter = rectangleVuiParameter(descriptor);
    if (!rectParameter) {
        QFile::remove(vuiPath);
        return true;
    }

    const AddOnParameterDescriptor *rotationParameter = rotationVuiParameter(descriptor);
    QFile file(vuiPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to open generated add-on vui.qml for writing");
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << "import Shotcut.Controls as Shotcut\n\n"
              "Shotcut.RectangleVui {\n"
              "    rectProperty: "
           << quotedJsString(rectParameter->name)
           << "\n"
              "    rotationProperty: "
           << quotedJsString(rotationParameter ? rotationParameter->name : QString())
           << "\n"
              "    rectKeyframes: "
           << (rectParameter->supportsKeyframes ? "true" : "false")
           << "\n"
              "    rotationKeyframes: "
           << ((rotationParameter && rotationParameter->supportsKeyframes) ? "true" : "false")
           << "\n"
              "    normalizedCoordinates: "
           << (rectParameter->normalizedCoordinates ? "true" : "false")
           << "\n"
              "}\n";
    file.close();
    return true;
}

static QStringList extraRectanglePropertyNames(const AddOnFilterDescriptor &descriptor)
{
    QStringList names;
    bool sawRectangle = false;
    for (const auto &parameter : descriptor.parameters) {
        if (!isRectangleVuiParameter(descriptor, parameter))
            continue;
        if (!sawRectangle) {
            sawRectangle = true;
            continue;
        }
        names << parameter.name;
    }
    return names;
}

static QString extraRectangleFlagLiteral(const QString &propertyName)
{
    return quotedJsString(QStringLiteral("shotcut:rectPixels:") + propertyName);
}

static void writeExtraRectangleFlagLines(QTextStream &stream,
                                         const AddOnFilterDescriptor &descriptor,
                                         const QString &indent,
                                         const QString &statement)
{
    for (const QString &name : extraRectanglePropertyNames(descriptor))
        stream << indent << statement.arg(extraRectangleFlagLiteral(name)) << "\n";
}

// Position and Size rows for a full rect that is not the player overlay.
static void writeExtraRectangleRows(QTextStream &stream,
                                    const AddOnParameterDescriptor &parameter,
                                    const QString &parameterId,
                                    const QString &nameLiteral,
                                    const QString &undoSpan)
{
    const QString xId = parameterId + QStringLiteral("_x");
    const QString yId = parameterId + QStringLiteral("_y");
    const QString wId = parameterId + QStringLiteral("_w");
    const QString hId = parameterId + QStringLiteral("_h");
    const QString keyframesId = parameterId + QStringLiteral("_keyframes");
    const QString commit = parameter.supportsKeyframes
                               ? QStringLiteral(
                                     "root.commitExtraRectangle(%1, %2.value, %3.value, %4.value, "
                                     "%5.value, %6.checked)")
                                     .arg(nameLiteral, xId, yId, wId, hId, keyframesId)
                               : QStringLiteral(
                                     "root.commitExtraRectangle(%1, %2.value, %3.value, %4.value, "
                                     "%5.value, false)")
                                     .arg(nameLiteral, xId, yId, wId, hId);

    stream << "        Label {\n"
              "            Layout.columnSpan: 4\n"
              "            Layout.fillWidth: true\n"
              "            font.bold: true\n"
              "            horizontalAlignment: Text.AlignHCenter\n"
              "            textFormat: Text.PlainText\n"
              "            text: (root.propertyTitles && root.propertyTitles["
           << nameLiteral << "]) ? root.propertyTitles[" << nameLiteral << "] : " << nameLiteral
           << "\n"
              "        }\n\n"
              "        Label {\n"
              "            text: qsTr('Position')\n"
              "            textFormat: Text.PlainText\n"
              "            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter\n"
              "        }\n\n"
              "        RowLayout {\n"
              "            Layout.columnSpan: 1\n"
              "            Layout.fillWidth: true\n\n"
              "            Shotcut.DoubleSpinBox {\n"
              "                id: "
           << xId
           << "\n"
              "                Layout.fillWidth: true\n"
              "                Layout.minimumWidth: 80\n"
              "                horizontalAlignment: Qt.AlignRight\n"
              "                decimals: 0\n"
              "                stepSize: 1\n"
              "                from: -1e+09\n"
              "                to: 1e+09\n"
              "                onValueModified: "
           << commit
           << "\n"
              "            }\n\n"
              "            Label {\n"
              "                text: ','\n"
              "                textFormat: Text.PlainText\n"
              "            }\n\n"
              "            Shotcut.DoubleSpinBox {\n"
              "                id: "
           << yId
           << "\n"
              "                Layout.fillWidth: true\n"
              "                Layout.minimumWidth: 80\n"
              "                horizontalAlignment: Qt.AlignRight\n"
              "                decimals: 0\n"
              "                stepSize: 1\n"
              "                from: -1e+09\n"
              "                to: 1e+09\n"
              "                onValueModified: "
           << commit
           << "\n"
              "            }\n"
              "        }\n\n"
              "        Shotcut.UndoButton {\n"
           << undoSpan
           << "            onClicked: {\n"
              "                var defaults = root.defaultRectanglePixels("
           << nameLiteral
           << ");\n"
              "                "
           << xId
           << ".value = defaults.x;\n"
              "                "
           << yId
           << ".value = defaults.y;\n"
              "                "
           << commit
           << ";\n"
              "            }\n"
              "        }\n\n";
    if (parameter.supportsKeyframes) {
        stream << "        ToolButton {\n"
                  "            id: "
               << keyframesId
               << "\n"
                  "            Layout.rowSpan: 2\n"
                  "            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter\n"
                  "            checkable: true\n"
                  "            padding: 2\n"
                  "            icon.name: 'chronometer'\n"
                  "            icon.source: 'qrc:///icons/oxygen/32x32/actions/chronometer.png'\n"
                  "            onClicked: {\n"
                  "                var rect = Qt.rect("
               << xId << ".value, " << yId << ".value, " << wId << ".value, " << hId
               << ".value);\n"
                  "                if (checked) {\n"
                  "                    root.writeExtraRect("
               << nameLiteral
               << ", rect, root.getPosition());\n"
                  "                } else {\n"
                  "                    filter.blockSignals = true;\n"
                  "                    filter.resetProperty("
               << nameLiteral
               << ");\n"
                  "                    filter.blockSignals = false;\n"
                  "                    root.writeExtraRect("
               << nameLiteral
               << ", rect, null);\n"
                  "                }\n"
                  "                checked = filter.keyframeCount("
               << nameLiteral
               << ") > 0;\n"
                  "            }\n"
                  "        }\n\n";
    }
    stream << "        Label {\n"
              "            text: qsTr('Size')\n"
              "            textFormat: Text.PlainText\n"
              "            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter\n"
              "        }\n\n"
              "        RowLayout {\n"
              "            Layout.columnSpan: 1\n"
              "            Layout.fillWidth: true\n\n"
              "            Shotcut.DoubleSpinBox {\n"
              "                id: "
           << wId
           << "\n"
              "                Layout.fillWidth: true\n"
              "                Layout.minimumWidth: 80\n"
              "                horizontalAlignment: Qt.AlignRight\n"
              "                decimals: 0\n"
              "                stepSize: 1\n"
              "                from: 0\n"
              "                to: 1e+09\n"
              "                onValueModified: "
           << commit
           << "\n"
              "            }\n\n"
              "            Label {\n"
              "                text: 'x'\n"
              "                textFormat: Text.PlainText\n"
              "            }\n\n"
              "            Shotcut.DoubleSpinBox {\n"
              "                id: "
           << hId
           << "\n"
              "                Layout.fillWidth: true\n"
              "                Layout.minimumWidth: 80\n"
              "                horizontalAlignment: Qt.AlignRight\n"
              "                decimals: 0\n"
              "                stepSize: 1\n"
              "                from: 0\n"
              "                to: 1e+09\n"
              "                onValueModified: "
           << commit
           << "\n"
              "            }\n"
              "        }\n\n"
              "        Shotcut.UndoButton {\n"
           << undoSpan
           << "            onClicked: {\n"
              "                var defaults = root.defaultRectanglePixels("
           << nameLiteral
           << ");\n"
              "                "
           << wId
           << ".value = defaults.width;\n"
              "                "
           << hId
           << ".value = defaults.height;\n"
              "                "
           << commit
           << ";\n"
              "            }\n"
              "        }\n\n";
    if (hasDividerLayoutHint(parameter)) {
        stream << "        Item {\n"
                  "            Layout.columnSpan: 4\n"
                  "            Layout.fillWidth: true\n"
                  "            Layout.preferredHeight: 8\n\n"
                  "            Rectangle {\n"
                  "                anchors.left: parent.left\n"
                  "                anchors.right: parent.right\n"
                  "                anchors.verticalCenter: parent.verticalCenter\n"
                  "                height: 1\n"
                  "                color: '#808080'\n"
                  "                opacity: 0.45\n"
                  "            }\n"
                  "        }\n\n";
    }
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
    QStringList quotedWidgets;
    QStringList quotedUnits;
    QStringList quotedMinimums;
    QStringList quotedMaximums;
    QStringList quotedDescriptions;
    QStringList quotedValueLists;
    QStringList quotedNormalizedCoordinates;
    QStringList quotedNormalizedDefault;
    QStringList quotedKeyframeProperties;
    QStringList quotedKeyframeMapEntries;
    QStringList setControlsLines;
    int parameterIndex = 0;
    QString rotationEditorId;
    QString rotationKeyframesId;
    for (const auto &parameter : descriptor.parameters) {
        if (isGroupHeadingParameter(parameter))
            continue;

        quotedProperties << quotedJsString(parameter.name);
        if (!parameter.title.isEmpty()) {
            quotedTitles << QStringLiteral("%1: qsTr(%2)")
                                .arg(quotedJsString(parameter.name),
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
        if (!parameter.widget.isEmpty()) {
            quotedWidgets << QStringLiteral("%1: %2").arg(quotedJsString(parameter.name),
                                                          quotedJsString(parameter.widget));
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
        if (parameter.normalizedCoordinates) {
            quotedNormalizedCoordinates
                << QStringLiteral("%1: 'yes'").arg(quotedJsString(parameter.name));
        }
        if (parameter.normalizedDefault) {
            quotedNormalizedDefault
                << QStringLiteral("%1: 'yes'").arg(quotedJsString(parameter.name));
        }
        if (!parameter.description.isEmpty()) {
            quotedDescriptions << QStringLiteral("%1: qsTr(%2)")
                                      .arg(quotedJsString(parameter.name),
                                           quotedJsString(parameter.description));
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
        const QString parameterWidget = parameter.widget.trimmed().toLower();
        const bool useTextForNumericEditor = (parameterType == QStringLiteral("integer")
                                              || parameterType == QStringLiteral("float"))
                                             && parameterWidget == QStringLiteral("text");
        const bool supportsGeneratedKeyframes = !parameter.isReadOnly && parameter.supportsKeyframes
                                                && (parameterType == QStringLiteral("integer")
                                                    || parameterType == QStringLiteral("float")
                                                    || parameterType == QStringLiteral("color"));
        const QString parameterId = QStringLiteral("param_%1").arg(parameterIndex++);
        const QString editorId = parameterId + QStringLiteral("_editor");
        const QString keyframesId = parameterId + QStringLiteral("_keyframes");
        const QString nameLiteral = quotedJsString(parameter.name);
        const bool useChannelMask = isChannelMaskParameter(descriptor, parameter);
        if (supportsGeneratedKeyframes) {
            quotedKeyframeProperties << nameLiteral;
            quotedKeyframeMapEntries << QStringLiteral("%1: true").arg(nameLiteral);
            if (rotationVuiParameter(descriptor) == &parameter) {
                rotationEditorId = editorId;
                rotationKeyframesId = keyframesId;
            }
        }
        const bool vuiRect = rectangleVuiParameter(descriptor) == &parameter;
        if (useChannelMask) {
            setControlsLines << QStringLiteral("        %1.setChannelsControls();").arg(editorId);
        } else if (parameter.isReadOnly) {
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
        } else if (parameterType == QStringLiteral("rect")
                   && (parameterWidget == QStringLiteral("point")
                       || parameterWidget == QStringLiteral("size"))) {
            setControlsLines
                << QStringLiteral(
                       "        { var _r = filter.getRect(%2); %1.valueX = isNaN(_r.x) ? 0 : "
                       "_r.x; %1.valueY = isNaN(_r.y) ? 0 : _r.y; }")
                       .arg(editorId, nameLiteral);
        } else if ((parameterType == QStringLiteral("integer")
                    || parameterType == QStringLiteral("float"))
                   && !useTextForNumericEditor) {
            setControlsLines
                << QStringLiteral(
                       "        %1.value = root.numericValue(%2, root.propertyType(%2));")
                       .arg(editorId, nameLiteral);
        } else if (vuiRect) {
            setControlsLines << QStringLiteral(
                "        var _rect = root.rectanglePixels(root.getPosition());\n"
                "        rectangleX.value = _rect.x;\n"
                "        rectangleY.value = _rect.y;\n"
                "        rectangleW.value = _rect.width;\n"
                "        rectangleH.value = _rect.height;\n"
                "        var _rectEnabled = !root.rectangleKeyframes || "
                "root.isSimpleKeyframesActive();\n"
                "        rectangleX.enabled = _rectEnabled;\n"
                "        rectangleY.enabled = _rectEnabled;\n"
                "        rectangleW.enabled = _rectEnabled;\n"
                "        rectangleH.enabled = _rectEnabled;");
            if (parameter.supportsKeyframes) {
                setControlsLines << QStringLiteral(
                    "        rectangleKeyframesButton.checked = filter.animateIn <= 0 && "
                    "filter.animateOut <= 0 && "
                    "filter.keyframeCount(root.rectangleProperty) > 0;");
            }
        } else if (isRectangleVuiParameter(descriptor, parameter)) {
            const QString xId = parameterId + QStringLiteral("_x");
            const QString yId = parameterId + QStringLiteral("_y");
            const QString wId = parameterId + QStringLiteral("_w");
            const QString hId = parameterId + QStringLiteral("_h");
            setControlsLines << QStringLiteral("        var %1_rect = root.extraRectPixels(%2, "
                                               "root.getPosition());\n"
                                               "        %3.value = %1_rect.x;\n"
                                               "        %4.value = %1_rect.y;\n"
                                               "        %5.value = %1_rect.width;\n"
                                               "        %6.value = %1_rect.height;")
                                    .arg(parameterId, nameLiteral, xId, yId, wId, hId);
            if (parameter.supportsKeyframes) {
                setControlsLines << QStringLiteral(
                                        "        %1.checked = filter.keyframeCount(%2) > 0;")
                                        .arg(keyframesId, nameLiteral);
            }
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
           "    property var propertyWidgets: {"
        << quotedWidgets.join(QStringLiteral(", "))
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
           "    property var propertyNormalizedCoordinates: {"
        << quotedNormalizedCoordinates.join(QStringLiteral(", "))
        << "}\n\n"
           "    property var propertyNormalizedDefault: {"
        << quotedNormalizedDefault.join(QStringLiteral(", "))
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
           "    function propertyWidget(name) {\n"
           "        if (!root.propertyWidgets)\n"
           "            return '';\n"
           "        var widget = root.propertyWidgets[name];\n"
           "        return (widget !== undefined && widget !== null) ? "
           "String(widget).toLowerCase() : '';\n"
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
           "    property string rectangleProperty: "
        << (rectangleVuiParameter(descriptor)
                ? quotedJsString(rectangleVuiParameter(descriptor)->name)
                : QStringLiteral("''"))
        << "\n"
           "    property var extraRectangleProperties: [";
    {
        QStringList extraNames;
        for (const QString &name : extraRectanglePropertyNames(descriptor))
            extraNames << quotedJsString(name);
        stream << extraNames.join(QStringLiteral(", "));
    }
    stream << "]\n"
              "    property bool rectangleKeyframes: "
           << ((rectangleVuiParameter(descriptor)
                && rectangleVuiParameter(descriptor)->supportsKeyframes)
                   ? "true\n"
                   : "false\n")
           << "    property bool rectangleChangeGuard: false\n"
              "    property string rectangleRotationProperty: "
           << (rotationVuiParameter(descriptor)
                       && rotationVuiParameter(descriptor)->supportsKeyframes
                   ? quotedJsString(rotationVuiParameter(descriptor)->name)
                   : QStringLiteral("''"))
           << "\n\n"
              "    function setControls() {\n"
              "        blockUpdate = true;\n";

    if (!setControlsLines.isEmpty()) {
        stream << setControlsLines.join(QStringLiteral("\n")) << "\n";
    }

    stream << "        blockUpdate = false;\n"
              "    }\n\n";
    if (rectangleVuiParameter(descriptor)) {
        stream << QStringLiteral(
            "    function rectangleStoredInPixels() {\n"
            "        var flag = filter.get('shotcut:rectPixels');\n"
            "        return flag === 1 || flag === '1';\n"
            "    }\n\n"
            "    function rectangleIsUnitless(rect) {\n"
            "        return Math.abs(rect.x) <= 1 && Math.abs(rect.y) <= 1 &&\n"
            "            Math.abs(rect.width) <= 1 && Math.abs(rect.height) <= 1;\n"
            "    }\n\n"
            "    function rectanglePixels(position) {\n"
            "        var name = root.rectangleProperty;\n"
            "        var value = filter.getRect(name, position);\n"
            "        var raw = filter.get(name);\n"
            "        var text = (raw === undefined || raw === null) ? '' : String(raw);\n"
            "        if (root.propertyNormalizedCoordinates &&\n"
            "                root.propertyNormalizedCoordinates[name] === 'yes' &&\n"
            "                text.indexOf('%') < 0 && !root.rectangleStoredInPixels() &&\n"
            "                root.rectangleIsUnitless(value)) {\n"
            "            value = Qt.rect(value.x * profile.width, value.y * profile.height,\n"
            "                value.width * profile.width, value.height * profile.height);\n"
            "        }\n"
            "        return value;\n"
            "    }\n\n"
            "    function convertRectangleKeyframesToPixels() {\n"
            "        if (root.rectangleStoredInPixels())\n"
            "            return;\n"
            "        var name = root.rectangleProperty;\n"
            "        var raw = filter.get(name);\n"
            "        var text = (raw === undefined || raw === null) ? '' : String(raw);\n"
            "        if (text.indexOf('%') >= 0) {\n"
            "            filter.blockSignals = true;\n"
            "            filter.removeRectPercents(name);\n"
            "            filter.set('shotcut:rectPixels', 1);\n"
            "            filter.blockSignals = false;\n"
            "            return;\n"
            "        }\n"
            "        if (!(root.propertyNormalizedCoordinates &&\n"
            "                root.propertyNormalizedCoordinates[name] === 'yes'))\n"
            "            return;\n"
            "        if (filter.keyframeCount(name) < 1)\n"
            "            return;\n"
            "        var keys = [];\n"
            "        var anyUnitless = false;\n"
            "        var pos = filter.getNextKeyframePosition(name, -1);\n"
            "        var guard = 0;\n"
            "        while (pos >= 0 && guard < 10000) {\n"
            "            var value = filter.getRect(name, pos);\n"
            "            var scaled = value;\n"
            "            if (root.rectangleIsUnitless(value)) {\n"
            "                anyUnitless = true;\n"
            "                scaled = Qt.rect(value.x * profile.width, value.y * profile.height,\n"
            "                    value.width * profile.width, value.height * profile.height);\n"
            "            }\n"
            "            keys.push({position: pos, rect: scaled});\n"
            "            var next = filter.getNextKeyframePosition(name, pos);\n"
            "            if (next === pos)\n"
            "                break;\n"
            "            pos = next;\n"
            "            guard++;\n"
            "        }\n"
            "        if (!anyUnitless)\n"
            "            return;\n"
            "        filter.blockSignals = true;\n"
            "        for (var i = 0; i < keys.length; ++i)\n"
            "            filter.set(name, keys[i].rect, keys[i].position);\n"
            "        filter.set('shotcut:rectPixels', 1);\n"
            "        filter.blockSignals = false;\n"
            "    }\n\n"
            "    function writeRectanglePixels(rect, position) {\n"
            "        if (position !== null)\n"
            "            root.convertRectangleKeyframesToPixels();\n"
            "        if (!root.rectangleStoredInPixels()) {\n"
            "            filter.blockSignals = true;\n"
            "            filter.set('shotcut:rectPixels', 1);\n"
            "            filter.blockSignals = false;\n"
            "        }\n"
            "        if (position === null)\n"
            "            filter.set(root.rectangleProperty, rect);\n"
            "        else\n"
            "            filter.set(root.rectangleProperty, rect, position);\n"
            "    }\n\n"
            "    function setRectangleFilter(position) {\n"
            "        var rect = Qt.rect(rectangleX.value, rectangleY.value, rectangleW.value, "
            "rectangleH.value);\n"
            "        if (!root.rectangleKeyframes) {\n"
            "            root.writeRectanglePixels(rect, null);\n"
            "            return;\n"
            "        }\n"
            "        filter.blockSignals = true;\n"
            "        if (position <= 0 && filter.animateIn > 0)\n"
            "            filter.set('_shotcut:startValue', rect);\n"
            "        else if (position >= filter.duration - 1 && filter.animateOut > 0)\n"
            "            filter.set('_shotcut:endValue', rect);\n"
            "        else\n"
            "            filter.set('_shotcut:middleValue', rect);\n"
            "        filter.blockSignals = false;\n"
            "        if (filter.animateIn > 0 || filter.animateOut > 0) {\n"
            "            filter.resetProperty(root.rectangleProperty);\n"
            "            if (filter.animateIn > 0) {\n"
            "                root.writeRectanglePixels(filter.getRect('_shotcut:startValue'), 0);\n"
            "                root.writeRectanglePixels(filter.getRect('_shotcut:middleValue'), "
            "filter.animateIn - 1);\n"
            "            }\n"
            "            if (filter.animateOut > 0) {\n"
            "                root.writeRectanglePixels(filter.getRect('_shotcut:middleValue'), "
            "filter.duration - filter.animateOut);\n"
            "                root.writeRectanglePixels(filter.getRect('_shotcut:endValue'), "
            "filter.duration - 1);\n"
            "            }\n"
            "        } else if (filter.keyframeCount(root.rectangleProperty) <= 0) {\n"
            "            filter.resetProperty(root.rectangleProperty);\n"
            "            root.writeRectanglePixels(filter.getRect('_shotcut:middleValue'), null);\n"
            "        } else if (position !== null) {\n"
            "            root.writeRectanglePixels(rect, position);\n"
            "        }\n"
            "    }\n\n"
            "    function rectangleComponentPixels(token, axisSize) {\n"
            "        var text = String(token === undefined || token === null ? '' : "
            "token).trim();\n"
            "        if (text.length === 0)\n"
            "            return 0;\n"
            "        if (text.charAt(text.length - 1) === '%') {\n"
            "            var percent = parseFloat(text);\n"
            "            return isNaN(percent) ? 0 : percent / 100 * axisSize;\n"
            "        }\n"
            "        var number = parseFloat(text);\n"
            "        return isNaN(number) ? 0 : number;\n"
            "    }\n\n"
            "    function defaultRectanglePixels(name) {\n"
            "        if (name === undefined || name === null || name === '')\n"
            "            name = root.rectangleProperty;\n"
            "        var d = root.propertyDefaults ? root.propertyDefaults[name] : '';\n"
            "        var text = String(d === undefined || d === null ? '' : d).trim();\n"
            "        var x = 0;\n"
            "        var y = 0;\n"
            "        var w = 0;\n"
            "        var h = 0;\n"
            "        var geometry = text.match(/^([^:]*):([^x]*)x(.*)$/);\n"
            "        if (geometry) {\n"
            "            var position = geometry[1].split('/');\n"
            "            var widthToken = geometry[2];\n"
            "            var heightToken = geometry[3];\n"
            "            var widthOpacity = widthToken.indexOf(':');\n"
            "            if (widthOpacity >= 0)\n"
            "                widthToken = widthToken.substring(0, widthOpacity);\n"
            "            var opacityAt = heightToken.indexOf(':');\n"
            "            if (opacityAt >= 0)\n"
            "                heightToken = heightToken.substring(0, opacityAt);\n"
            "            x = root.rectangleComponentPixels(position[0], profile.width);\n"
            "            y = root.rectangleComponentPixels(position.length > 1 ? position[1] : "
            "'0', "
            "profile.height);\n"
            "            w = root.rectangleComponentPixels(widthToken, profile.width);\n"
            "            h = root.rectangleComponentPixels(heightToken, profile.height);\n"
            "        } else {\n"
            "            var parts = text.split(/\\s+/);\n"
            "            x = root.rectangleComponentPixels(parts.length > 0 ? parts[0] : '0', "
            "profile.width);\n"
            "            y = root.rectangleComponentPixels(parts.length > 1 ? parts[1] : '0', "
            "profile.height);\n"
            "            w = root.rectangleComponentPixels(parts.length > 2 ? parts[2] : '0', "
            "profile.width);\n"
            "            h = root.rectangleComponentPixels(parts.length > 3 ? parts[3] : '0', "
            "profile.height);\n"
            "        }\n"
            "        var normalized = root.propertyNormalizedDefault &&\n"
            "                root.propertyNormalizedDefault[name] === 'yes';\n"
            "        if (normalized && text.indexOf('%') < 0) {\n"
            "            x *= profile.width;\n"
            "            y *= profile.height;\n"
            "            w *= profile.width;\n"
            "            h *= profile.height;\n"
            "        }\n"
            "        return Qt.rect(x, y, w, h);\n"
            "    }\n\n"
            "    function rewriteRectangleKeyframes() {\n"
            "        if (!root.rectangleKeyframes)\n"
            "            return;\n"
            "        if (filter.animateIn > 0 || filter.animateOut > 0) {\n"
            "            filter.resetProperty(root.rectangleProperty);\n"
            "            if (filter.animateIn > 0) {\n"
            "                root.writeRectanglePixels(filter.getRect('_shotcut:startValue'), 0);\n"
            "                root.writeRectanglePixels(filter.getRect('_shotcut:middleValue'), "
            "filter.animateIn - 1);\n"
            "            }\n"
            "            if (filter.animateOut > 0) {\n"
            "                root.writeRectanglePixels(filter.getRect('_shotcut:middleValue'), "
            "filter.duration - filter.animateOut);\n"
            "                root.writeRectanglePixels(filter.getRect('_shotcut:endValue'), "
            "filter.duration - 1);\n"
            "            }\n"
            "        } else if (filter.keyframeCount(root.rectangleProperty) <= 0) {\n"
            "            filter.resetProperty(root.rectangleProperty);\n"
            "            root.writeRectanglePixels(filter.getRect('_shotcut:middleValue'), null);\n"
            "        }\n"
            "    }\n\n");
        if (!extraRectanglePropertyNames(descriptor).isEmpty()) {
            stream << QStringLiteral(
                "    function extraRectFlag(name) {\n"
                "        return 'shotcut:rectPixels:' + name;\n"
                "    }\n\n"
                "    function extraRectStoredInPixels(name) {\n"
                "        var flag = filter.get(root.extraRectFlag(name));\n"
                "        return flag === 1 || flag === '1';\n"
                "    }\n\n"
                "    function extraRectPixels(name, position) {\n"
                "        var value = filter.getRect(name, position);\n"
                "        var raw = filter.get(name);\n"
                "        var text = (raw === undefined || raw === null) ? '' : String(raw);\n"
                "        if (root.propertyNormalizedCoordinates &&\n"
                "                root.propertyNormalizedCoordinates[name] === 'yes' &&\n"
                "                text.indexOf('%') < 0 && !root.extraRectStoredInPixels(name) &&\n"
                "                root.rectangleIsUnitless(value)) {\n"
                "            value = Qt.rect(value.x * profile.width, value.y * profile.height,\n"
                "                value.width * profile.width, value.height * profile.height);\n"
                "        }\n"
                "        return value;\n"
                "    }\n\n"
                "    function convertExtraRectKeyframes(name) {\n"
                "        if (root.extraRectStoredInPixels(name))\n"
                "            return;\n"
                "        var raw = filter.get(name);\n"
                "        var text = (raw === undefined || raw === null) ? '' : String(raw);\n"
                "        if (text.indexOf('%') >= 0) {\n"
                "            filter.blockSignals = true;\n"
                "            filter.removeRectPercents(name);\n"
                "            filter.set(root.extraRectFlag(name), 1);\n"
                "            filter.blockSignals = false;\n"
                "            return;\n"
                "        }\n"
                "        if (!(root.propertyNormalizedCoordinates &&\n"
                "                root.propertyNormalizedCoordinates[name] === 'yes'))\n"
                "            return;\n"
                "        if (filter.keyframeCount(name) < 1)\n"
                "            return;\n"
                "        var keys = [];\n"
                "        var anyUnitless = false;\n"
                "        var pos = filter.getNextKeyframePosition(name, -1);\n"
                "        var guard = 0;\n"
                "        while (pos >= 0 && guard < 10000) {\n"
                "            var value = filter.getRect(name, pos);\n"
                "            var scaled = value;\n"
                "            if (root.rectangleIsUnitless(value)) {\n"
                "                anyUnitless = true;\n"
                "                scaled = Qt.rect(value.x * profile.width, "
                "value.y * profile.height,\n"
                "                    value.width * profile.width, value.height * profile.height);\n"
                "            }\n"
                "            keys.push({position: pos, rect: scaled});\n"
                "            var next = filter.getNextKeyframePosition(name, pos);\n"
                "            if (next === pos)\n"
                "                break;\n"
                "            pos = next;\n"
                "            guard++;\n"
                "        }\n"
                "        if (!anyUnitless)\n"
                "            return;\n"
                "        filter.blockSignals = true;\n"
                "        for (var i = 0; i < keys.length; ++i)\n"
                "            filter.set(name, keys[i].rect, keys[i].position);\n"
                "        filter.set(root.extraRectFlag(name), 1);\n"
                "        filter.blockSignals = false;\n"
                "    }\n\n"
                "    function writeExtraRect(name, rect, position) {\n"
                "        if (position !== null)\n"
                "            root.convertExtraRectKeyframes(name);\n"
                "        if (!root.extraRectStoredInPixels(name)) {\n"
                "            filter.blockSignals = true;\n"
                "            filter.set(root.extraRectFlag(name), 1);\n"
                "            filter.blockSignals = false;\n"
                "        }\n"
                "        if (position === null)\n"
                "            filter.set(name, rect);\n"
                "        else\n"
                "            filter.set(name, rect, position);\n"
                "    }\n\n"
                "    function commitExtraRectangle(name, x, y, w, h, atPosition) {\n"
                "        var rect = Qt.rect(x, y, w, h);\n"
                "        root.writeExtraRect(name, rect, atPosition ? root.getPosition() : null);\n"
                "    }\n\n");
        }
        if (!rotationEditorId.isEmpty()) {
            stream << QStringLiteral(
                "    function consumeVuiRotation() {\n"
                "        if (!root.rectangleRotationProperty)\n"
                "            return;\n"
                "        var raw = filter.get('_shotcut:vuiRotation');\n"
                "        if (raw === undefined || raw === null || raw === '')\n"
                "            return;\n"
                "        var value = Number(raw);\n"
                "        var pos = Number(filter.get('_shotcut:vuiRotationPos'));\n"
                "        filter.blockSignals = true;\n"
                "        filter.set('_shotcut:vuiRotation', '');\n"
                "        filter.set('_shotcut:vuiRotationPos', '');\n"
                "        filter.blockSignals = false;\n"
                "        if (isNaN(value))\n"
                "            return;\n"
                "        var position = (isNaN(pos) || pos < 0) ? null : pos;\n"
                "        var widget = root.propertyWidget(root.rectangleRotationProperty);\n"
                "        root.blockUpdate = true;\n"
                "        if (widget === 'text')\n"
                "            ")
                   << rotationEditorId
                   << QStringLiteral(".text = String(value);\n"
                                     "        else\n"
                                     "            ")
                   << rotationEditorId
                   << QStringLiteral(
                          ".value = value;\n"
                          "        root.blockUpdate = false;\n"
                          "        root.updateFilter(root.rectangleRotationProperty, value, ")
                   << rotationKeyframesId
                   << QStringLiteral(
                          ", position);\n"
                          "    }\n\n"
                          "    function rewriteRotationKeyframes() {\n"
                          "        var index = root.keyframableParameters.indexOf(root."
                          "rectangleRotationProperty);\n"
                          "        if (index >= 0) {\n"
                          "            if (root.startValues[index] === undefined)\n"
                          "                root.startValues[index] = filter.getDouble(root."
                          "rectangleRotationProperty, 0);\n"
                          "            if (root.endValues[index] === undefined)\n"
                          "                root.endValues[index] = filter.getDouble(root."
                          "rectangleRotationProperty, Math.max(filter.duration - 1, 0));\n"
                          "        }\n"
                          "        var current = filter.getDouble(root.rectangleRotationProperty, "
                          "root.getPosition());\n"
                          "        root.updateFilter(root.rectangleRotationProperty, current, ")
                   << rotationKeyframesId
                   << QStringLiteral(", null);\n"
                                     "    }\n\n");
        }
    }
    stream
        << "    width: 360\n"
           "    height: gridLayout.implicitHeight + 16\n\n"
           "    Component.onCompleted: {\n"
           "        if (filter.isNew) {\n"
           "            for (var i = 0; i < propertyNames.length; ++i) {\n"
           "                var p = propertyNames[i];\n"
           "                var d = root.propertyDefaults[p];\n"
           "                var type = propertyType(p);\n"
           "                var widget = propertyWidget(p);\n"
           "                if (d !== undefined && d !== null && d !== '') {\n"
           "                    var _positionRect = (root.rectangleProperty && p === "
           "root.rectangleProperty) ||\n"
           "                        (root.extraRectangleProperties && "
           "root.extraRectangleProperties.indexOf(p) >= 0);\n"
           "                    if (type === 'rect' && widget !== 'point' && widget !== 'size' && "
           "_positionRect) {\n"
           "                        var _rect = root.defaultRectanglePixels(p);\n"
           "                        filter.set(p, _rect.x + ' ' + _rect.y + ' ' + _rect.width + ' "
           "' + _rect.height + ' 1');\n"
           "                        if (root.rectangleProperty && p === root.rectangleProperty)\n"
           "                            filter.set('shotcut:rectPixels', 1);\n"
           "                        else\n"
           "                            filter.set(root.extraRectFlag(p), 1);\n"
           "                    } else if (type === 'rect' && root.propertyNormalizedDefault\n"
           "                            && root.propertyNormalizedDefault[p] === 'yes') {\n"
           "                        var _dp = String(d).trim().split(/\\s+/);\n"
           "                        var _dx = _dp.length > 0 ? parseFloat(_dp[0]) : 0;\n"
           "                        var _dy = _dp.length > 1 ? parseFloat(_dp[1]) : 0;\n"
           "                        var _dw = _dp.length > 2 ? parseFloat(_dp[2]) : 0;\n"
           "                        var _dh = _dp.length > 3 ? parseFloat(_dp[3]) : 0;\n"
           "                        if (isNaN(_dx)) _dx = 0;\n"
           "                        if (isNaN(_dy)) _dy = 0;\n"
           "                        if (isNaN(_dw)) _dw = 0;\n"
           "                        if (isNaN(_dh)) _dh = 0;\n"
           "                        _dx *= profile.width;\n"
           "                        _dy *= profile.height;\n"
           "                        if (widget === 'point' || widget === 'size') {\n"
           "                            filter.set(p, _dx + ' ' + _dy + ' 0 0 0');\n"
           "                        } else if (root.propertyNormalizedCoordinates &&\n"
           "                                root.propertyNormalizedCoordinates[p] === 'yes') {\n"
           "                            filter.set(p, d);\n"
           "                        } else {\n"
           "                            _dw *= profile.width;\n"
           "                            _dh *= profile.height;\n"
           "                            filter.set(p, _dx + ' ' + _dy + ' ' + _dw + ' ' + _dh + ' "
           "1');\n"
           "                        }\n"
           "                    } else {\n"
           "                        filter.set(p, isBooleanType(type) ? (booleanValue(p) ? '1' : "
           "'0') "
           ": ((isNumericType(type) && widget === 'text') ? String(d) : (isNumericType(type) ? "
           "Number(d) : d)));\n"
           "                    }\n"
           "                }\n"
           "            }\n"
           "            var _presetNames = propertyNames.slice(0);\n"
           "            if (root.rectangleProperty)\n"
           "                _presetNames.push('shotcut:rectPixels');\n";
    writeExtraRectangleFlagLines(stream,
                                 descriptor,
                                 QStringLiteral("                "),
                                 QStringLiteral("_presetNames.push(%1);"));
    stream << "            filter.savePreset(_presetNames);\n"
              "        }\n"
              "        setControls();\n";
    const QString rectangleSeed = rectanglePresetSeedJs(descriptor);
    // Block signals only here. Preset.qml already blocks them around onPresetSelected,
    // and clearing that flag inside the preset handler would unblock the rest of the load.
    if (!rectangleSeed.isEmpty()) {
        stream << QStringLiteral("        filter.blockSignals = true;\n") << rectangleSeed
               << QStringLiteral("        filter.blockSignals = false;\n");
    }
    stream << QStringLiteral("        if (keyframableParameters.length > 0)\n"
                             "            initializeSimpleKeyframes();\n"
                             "    }\n\n")
           << "    GridLayout {\n"
              "        id: gridLayout\n"
              "        anchors.left: parent.left\n"
              "        anchors.right: parent.right\n"
              "        anchors.top: parent.top\n"
              "        anchors.margins: 8\n"
              "        columns: 4\n"
              "        columnSpacing: 8\n"
              "        rowSpacing: 6\n\n"
              "        Label {\n"
              "            id: descriptionText\n"
              "            Layout.columnSpan: 3\n"
              "            Layout.preferredWidth: root.width - helpButton.implicitWidth - "
              "parent.anchors.margins * 2\n"
              "            maximumLineCount: 1\n"
              "            elide: Text.ElideRight\n"
              "            text: root.filterDescription.length > 0 ? qsTr('Add-on Filter: "
              "%1').arg(root.filterDescription) : qsTr('Add-on Filter')\n"
              "            textFormat: Text.PlainText\n\n"
              "            Shotcut.HoverTip {\n"
              "                visible: descriptionText.truncated\n"
              "                text: root.filterDescription\n"
              "            }\n"
              "        }\n\n"
              "        ToolButton {\n"
              "            id: helpButton\n"
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
              "            textFormat: Text.PlainText\n"
              "            horizontalAlignment: Text.AlignRight\n"
              "            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter\n"
              "        }\n"
              "\n"
              "        Shotcut.Preset {\n"
              "            id: preset\n"
              "            Layout.columnSpan: 3\n"
              "            Layout.fillWidth: true\n"
              "            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter\n"
              "            parameters: {\n"
              "                var names = root.propertyNames ? root.propertyNames.slice(0) : [];\n"
              "                if (root.rectangleProperty)\n"
              "                    names.push('shotcut:rectPixels');\n";
    writeExtraRectangleFlagLines(stream,
                                 descriptor,
                                 QStringLiteral("                    "),
                                 QStringLiteral("names.push(%1);"));
    stream << "                return names;\n"
              "            }\n"
              "            onBeforePresetLoaded: {\n"
              "                filter.resetProperty('shotcut:animIn');\n"
              "                filter.resetProperty('shotcut:animOut');\n"
              "                if (root.rectangleProperty)\n"
              "                    filter.resetProperty('shotcut:rectPixels');\n";
    writeExtraRectangleFlagLines(stream,
                                 descriptor,
                                 QStringLiteral("                    "),
                                 QStringLiteral("filter.resetProperty(%1);"));
    stream
        << "                if (keyframableParameters.length > 0)\n"
           "                    resetSimpleKeyframes();\n"
           "                for (var i = 0; i < root.propertyNames.length; ++i)\n"
           "                    filter.resetProperty(root.propertyNames[i]);\n"
           "            }\n"
           "            onPresetSelected: {\n"
           "                filter.animateIn = Math.round(filter.getDouble('shotcut:animIn'));\n"
           "                filter.animateOut = Math.round(filter.getDouble('shotcut:animOut'));\n"
        << rectangleSeed
        << "                root.setControls();\n"
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
                      "            textFormat: Text.PlainText\n"
                      "            text: "
                   << quotedJsString(sectionTitle)
                   << "\n"
                      "        }\n\n";
            continue;
        }

        const QString parameterType = parameter.type.trimmed().toLower();
        const QString parameterWidget = parameter.widget.trimmed().toLower();
        const bool useTextForNumericEditor = (parameterType == QStringLiteral("integer")
                                              || parameterType == QStringLiteral("float"))
                                             && parameterWidget == QStringLiteral("text");
        const bool supportsGeneratedKeyframes = !parameter.isReadOnly && parameter.supportsKeyframes
                                                && (parameterType == QStringLiteral("integer")
                                                    || parameterType == QStringLiteral("float")
                                                    || parameterType == QStringLiteral("color"));
        const QString parameterId = QStringLiteral("param_%1").arg(rowIndex++);
        const QString hoverId = parameterId + QStringLiteral("_hover");
        const QString editorId = parameterId + QStringLiteral("_editor");
        const QString keyframesId = parameterId + QStringLiteral("_keyframes");

        const QString nameLiteral = quotedJsString(parameter.name);
        const bool useChannelMask = isChannelMaskParameter(descriptor, parameter);
        const bool vuiRect = rectangleVuiParameter(descriptor) == &parameter;
        // Keep Undo in the same column as the other parameter rows.
        const QString rectangleUndoSpan = parameter.supportsKeyframes
                                              ? QString()
                                              : QStringLiteral(
                                                  "            Layout.columnSpan: 2\n");

        if (vuiRect) {
            stream
                << "        Label {\n"
                   "            text: qsTr('Position')\n"
                   "            textFormat: Text.PlainText\n"
                   "            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter\n"
                   "        }\n\n"
                   "        RowLayout {\n"
                   "            Layout.columnSpan: 1\n"
                   "            Layout.fillWidth: true\n\n"
                   "            Shotcut.DoubleSpinBox {\n"
                   "                id: rectangleX\n"
                   "                Layout.fillWidth: true\n"
                   "                Layout.minimumWidth: 80\n"
                   "                horizontalAlignment: Qt.AlignRight\n"
                   "                decimals: 0\n"
                   "                stepSize: 1\n"
                   "                from: -1e+09\n"
                   "                to: 1e+09\n"
                   "                onValueModified: root.setRectangleFilter(root.getPosition())\n"
                   "            }\n\n"
                   "            Label {\n"
                   "                text: ','\n"
                   "                textFormat: Text.PlainText\n"
                   "            }\n\n"
                   "            Shotcut.DoubleSpinBox {\n"
                   "                id: rectangleY\n"
                   "                Layout.fillWidth: true\n"
                   "                Layout.minimumWidth: 80\n"
                   "                horizontalAlignment: Qt.AlignRight\n"
                   "                decimals: 0\n"
                   "                stepSize: 1\n"
                   "                from: -1e+09\n"
                   "                to: 1e+09\n"
                   "                onValueModified: root.setRectangleFilter(root.getPosition())\n"
                   "            }\n"
                   "        }\n\n"
                   "        Shotcut.UndoButton {\n"
                << rectangleUndoSpan
                << "            onClicked: {\n"
                   "                var defaults = root.defaultRectanglePixels();\n"
                   "                rectangleX.value = defaults.x;\n"
                   "                rectangleY.value = defaults.y;\n"
                   "                root.setRectangleFilter(root.getPosition());\n"
                   "            }\n"
                   "        }\n\n";
            if (parameter.supportsKeyframes) {
                stream
                    << "        Shotcut.KeyframesButton {\n"
                       "            id: rectangleKeyframesButton\n"
                       "            Layout.rowSpan: 2\n"
                       "            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter\n"
                       "            onToggled: {\n"
                       "                var rect = Qt.rect(rectangleX.value, rectangleY.value, "
                       "rectangleW.value, rectangleH.value);\n"
                       "                if (checked) {\n"
                       "                    root.blockUpdate = true;\n"
                       "                    filter.blockSignals = true;\n"
                       "                    filter.clearSimpleAnimation(root.rectangleProperty);\n"
                       "                    filter.blockSignals = false;\n"
                       "                    root.blockUpdate = false;\n"
                       "                    root.writeRectanglePixels(rect, root.getPosition());\n"
                       "                } else {\n"
                       "                    filter.blockSignals = true;\n"
                       "                    filter.resetProperty(root.rectangleProperty);\n"
                       "                    filter.blockSignals = false;\n"
                       "                    root.writeRectanglePixels(rect, null);\n"
                       "                }\n"
                       "                checked = filter.keyframeCount(root.rectangleProperty) > "
                       "0 && filter.animateIn <= 0 && filter.animateOut <= 0;\n"
                       "            }\n"
                       "        }\n\n";
            }
            stream
                << "        Label {\n"
                   "            text: qsTr('Size')\n"
                   "            textFormat: Text.PlainText\n"
                   "            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter\n"
                   "        }\n\n"
                   "        RowLayout {\n"
                   "            Layout.columnSpan: 1\n"
                   "            Layout.fillWidth: true\n\n"
                   "            Shotcut.DoubleSpinBox {\n"
                   "                id: rectangleW\n"
                   "                Layout.fillWidth: true\n"
                   "                Layout.minimumWidth: 80\n"
                   "                horizontalAlignment: Qt.AlignRight\n"
                   "                decimals: 0\n"
                   "                stepSize: 1\n"
                   "                from: 0\n"
                   "                to: 1e+09\n"
                   "                onValueModified: root.setRectangleFilter(root.getPosition())\n"
                   "            }\n\n"
                   "            Label {\n"
                   "                text: 'x'\n"
                   "                textFormat: Text.PlainText\n"
                   "            }\n\n"
                   "            Shotcut.DoubleSpinBox {\n"
                   "                id: rectangleH\n"
                   "                Layout.fillWidth: true\n"
                   "                Layout.minimumWidth: 80\n"
                   "                horizontalAlignment: Qt.AlignRight\n"
                   "                decimals: 0\n"
                   "                stepSize: 1\n"
                   "                from: 0\n"
                   "                to: 1e+09\n"
                   "                onValueModified: root.setRectangleFilter(root.getPosition())\n"
                   "            }\n"
                   "        }\n\n"
                   "        Shotcut.UndoButton {\n"
                << rectangleUndoSpan
                << "            onClicked: {\n"
                   "                var defaults = root.defaultRectanglePixels();\n"
                   "                rectangleW.value = defaults.width;\n"
                   "                rectangleH.value = defaults.height;\n"
                   "                root.setRectangleFilter(root.getPosition());\n"
                   "            }\n"
                   "        }\n\n";
            if (hasDividerLayoutHint(parameter)) {
                stream << "        Item {\n"
                          "            Layout.columnSpan: 4\n"
                          "            Layout.fillWidth: true\n"
                          "            Layout.preferredHeight: 8\n\n"
                          "            Rectangle {\n"
                          "                anchors.left: parent.left\n"
                          "                anchors.right: parent.right\n"
                          "                anchors.verticalCenter: parent.verticalCenter\n"
                          "                height: 1\n"
                          "                color: '#808080'\n"
                          "                opacity: 0.45\n"
                          "            }\n"
                          "        }\n\n";
            }
            continue;
        }

        if (isRectangleVuiParameter(descriptor, parameter)) {
            writeExtraRectangleRows(stream, parameter, parameterId, nameLiteral, rectangleUndoSpan);
            continue;
        }

        if (parameter.hideLabel) {
        } else if (parameterType == QStringLiteral("boolean") && !parameter.isReadOnly) {
            stream << "        Item {}\n\n";
        } else {
            stream << "        Label {\n"
                      "            text: (root.propertyTitles && root.propertyTitles["
                   << nameLiteral << "]) ? root.propertyTitles[" << nameLiteral
                   << "] : " << nameLiteral
                   << "\n"
                      "            textFormat: Text.PlainText\n"
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

        if (useChannelMask) {
            stream << "        Shotcut.ChannelMask {\n"
                      "            id: "
                   << editorId
                   << "\n"
                      ""
                      "            Layout.columnSpan: "
                   << (parameter.hideLabel ? "3" : "2")
                   << "\n"
                      "            readonly property string propertyName: "
                   << nameLiteral
                   << "\n"
                      "            channelMaskProperty: propertyName\n"
                      "        }\n";
        } else if (parameter.isReadOnly && parameter.hideLabel) {
            stream << "        TextEdit {\n"
                      "            id: "
                   << editorId
                   << "\n"
                      ""
                      "            Layout.columnSpan: 4\n"
                      "            Layout.fillWidth: true\n"
                      "            Layout.preferredHeight: contentHeight\n"
                      "            Layout.alignment: Qt.AlignLeft | Qt.AlignTop\n"
                      "            readonly property string propertyName: "
                   << nameLiteral
                   << "\n"
                      "            textFormat: TextEdit.PlainText\n"
                      "            text: ''\n"
                      "            wrapMode: TextEdit.Wrap\n"
                      "            readOnly: true\n"
                      "            selectByMouse: true\n"
                      "            color: activePalette.windowText\n"
                      "        }\n";
        } else if (parameter.isReadOnly) {
            stream << "        TextEdit {\n"
                      "            id: "
                   << editorId
                   << "\n"
                      ""
                      "            Layout.columnSpan: 3\n"
                      "            Layout.fillWidth: true\n"
                      "            Layout.preferredHeight: contentHeight\n"
                      "            Layout.alignment: Qt.AlignLeft | Qt.AlignTop\n"
                      "            readonly property string propertyName: "
                   << nameLiteral
                   << "\n"
                      "            textFormat: TextEdit.PlainText\n"
                      "            text: ''\n"
                      "            wrapMode: TextEdit.Wrap\n"
                      "            readOnly: true\n"
                      "            selectByMouse: true\n"
                      "            color: activePalette.windowText\n"
                      "        }\n";
        } else if (parameterType == QStringLiteral("string") && !parameter.values.isEmpty()) {
            stream << "        ComboBox {\n"
                      "            id: "
                   << editorId
                   << "\n"
                      ""
                      "            Layout.columnSpan: "
                   << (parameter.hideLabel ? "2" : "1")
                   << "\n"
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
                      "            Layout.columnSpan: "
                   << (parameter.hideLabel ? "2" : "1")
                   << "\n"
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
                   "            Layout.columnSpan: "
                << (parameter.hideLabel ? "2" : "1")
                << "\n"
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
        } else if (parameterType == QStringLiteral("rect")
                   && (parameterWidget == QStringLiteral("point")
                       || parameterWidget == QStringLiteral("size"))) {
            const QString labelFirst = (parameterWidget == QStringLiteral("size"))
                                           ? QStringLiteral("W")
                                           : QStringLiteral("X");
            const QString labelSecond = (parameterWidget == QStringLiteral("size"))
                                            ? QStringLiteral("H")
                                            : QStringLiteral("Y");
            stream
                << "        Shotcut.Number2D {\n"
                   "            id: "
                << editorId
                << "\n"
                   "            Layout.columnSpan: "
                << (parameter.hideLabel ? "2" : "1")
                << "\n"
                   "            Layout.fillWidth: true\n"
                   "            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter\n"
                   "            readonly property string propertyName: "
                << nameLiteral
                << "\n"
                   "            readonly property string typeName: "
                   "root.propertyType(propertyName)\n"
                   "            decimals: root.isIntegerType(typeName) ? 0 : 3\n"
                   "            from: (root.propertyMinimums && "
                   "root.propertyMinimums[propertyName] !== undefined && "
                   "root.propertyMinimums[propertyName] !== '') ? "
                   "Number(root.propertyMinimums[propertyName]) : -99999\n"
                   "            to: (root.propertyMaximums && root.propertyMaximums[propertyName] "
                   "!== undefined && root.propertyMaximums[propertyName] !== '') ? "
                   "Number(root.propertyMaximums[propertyName]) : 99999\n"
                   "            labelFirst: "
                << quotedJsString(labelFirst)
                << "\n"
                   "            labelSecond: "
                << quotedJsString(labelSecond)
                << "\n"
                   "            onValuesModified: {\n"
                   "                filter.set(propertyName, valueX + ' ' + valueY + ' 0 0 1');\n"
                   "            }\n"
                   "        }\n";
        } else if ((parameterType == QStringLiteral("integer")
                    || parameterType == QStringLiteral("float"))
                   && !useTextForNumericEditor) {
            stream << "        Shotcut.SliderSpinner {\n"
                      "            id: "
                   << editorId
                   << "\n"
                      ""
                      "            Layout.columnSpan: "
                   << (parameter.hideLabel ? "2" : "1")
                   << "\n"
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
                      "            Layout.columnSpan: "
                   << (parameter.hideLabel ? "2" : "1")
                   << "\n"
                      "            Layout.fillWidth: true\n"
                      "            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter\n"
                      "            selectByMouse: true\n"
                      "            readonly property string propertyName: "
                   << nameLiteral
                   << "\n"
                      "            readonly property string typeName: "
                      "root.propertyType(propertyName)\n";

            stream << "            text: {\n"
                      "                var value = filter.get(propertyName);\n"
                      "                if (value !== undefined && value !== null && value !== '')\n"
                      "                    return value;\n"
                      "                var d = root.propertyDefaults[propertyName];\n"
                      "                return (d !== undefined && d !== null) ? d : '';\n"
                      "            }\n"
                      "            onEditingFinished: {\n";

            if (useTextForNumericEditor) {
                stream << "                var nextValue = text.trim();\n"
                          "                if (nextValue === '') {\n"
                          "                    root.setControls();\n"
                          "                    return;\n"
                          "                }\n"
                          "                if (typeName === 'integer') {\n"
                          "                    var integerOrHex = "
                          "/^[-+]?(?:\\d+|0[xX][0-9A-Fa-f]+)$/;\n"
                          "                    if (!integerOrHex.test(nextValue)) {\n"
                          "                        root.setControls();\n"
                          "                        return;\n"
                          "                    }\n"
                          "                } else if (typeName === 'float') {\n"
                          "                    var parsedFloat = Number(nextValue);\n"
                          "                    if (isNaN(parsedFloat)) {\n"
                          "                        root.setControls();\n"
                          "                        return;\n"
                          "                    }\n"
                          "                    nextValue = String(parsedFloat);\n"
                          "                }\n"
                          "                if (root.isKeyframableProperty(propertyName)) {\n"
                          "                    var numericValue = Number(nextValue);\n"
                          "                    if (!isNaN(numericValue))\n"
                          "                        root.updateFilter(propertyName, numericValue, "
                       << keyframesId
                       << ", root.getPosition());\n"
                          "                } else {\n"
                          "                    var current = String(filter.get(propertyName));\n"
                          "                    if (current !== nextValue)\n"
                          "                        filter.set(propertyName, nextValue);\n"
                          "                }\n";
            } else {
                stream << "                if (root.isKeyframableProperty(propertyName)) {\n"
                          "                    root.updateFilter(propertyName, text, "
                       << keyframesId
                       << ", root.getPosition());\n"
                          "                } else {\n"
                          "                    var current = filter.get(propertyName);\n"
                          "                    if (String(current) !== text)\n"
                          "                        filter.set(propertyName, text);\n"
                          "                }\n";
            }

            stream << "            }\n"
                      "        }\n";
        }

        if (!parameter.isReadOnly && !useChannelMask) {
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
                if (useTextForNumericEditor) {
                    stream << "                var defaultValue = "
                              "root.defaultTextValue(propertyName);\n"
                              "                if (root.isKeyframableProperty(propertyName))\n"
                              "                    root.updateFilter(propertyName, "
                              "Number(defaultValue), "
                           << keyframesId
                           << ", root.getPosition());\n"
                              "                else\n"
                              "                    filter.set(propertyName, defaultValue);\n"
                              "                root.setControls();\n";
                } else {
                    stream << "                var defaultValue = "
                              "root.defaultNumericValue(propertyName, "
                              "typeName);\n"
                              "                if (root.isKeyframableProperty(propertyName))\n"
                              "                    root.updateFilter(propertyName, defaultValue, "
                           << keyframesId
                           << ", root.getPosition());\n"
                              "                else\n"
                              "                    filter.set(propertyName, defaultValue);\n"
                              "                root.setControls();\n";
                }
            } else if (parameterType == QStringLiteral("rect")
                       && (parameterWidget == QStringLiteral("point")
                           || parameterWidget == QStringLiteral("size"))) {
                stream << "                var _ds = root.propertyDefaults ? "
                          "(root.propertyDefaults[propertyName] || '') : '';\n"
                          "                var _dp = String(_ds).trim().split(/\\s+/);\n"
                          "                var _dx = _dp.length > 0 ? parseFloat(_dp[0]) : 0;\n"
                          "                var _dy = _dp.length > 1 ? parseFloat(_dp[1]) : 0;\n"
                          "                if (isNaN(_dx)) _dx = 0;\n"
                          "                if (isNaN(_dy)) _dy = 0;\n"
                          "                if (root.propertyNormalizedDefault &&\n"
                          "root.propertyNormalizedDefault[propertyName] === 'yes') {\n"
                          "                    _dx *= profile.width;\n"
                          "                    _dy *= profile.height;\n"
                          "                }\n"
                          "                filter.set(propertyName, _dx + ' ' + _dy + ' 0 0 0');\n"
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
                    if (useTextForNumericEditor) {
                        stream << "                var numericValue = Number(" << editorId
                               << ".text);\n"
                                  "                if (!isNaN(numericValue))\n"
                                  "                    root.toggleKeyframes(checked, propertyName, "
                                  "numericValue);\n";
                    } else {
                        stream << "                root.toggleKeyframes(checked, propertyName, "
                               << editorId << ".value);\n";
                    }
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

        if (hasDividerLayoutHint(parameter)) {
            stream << "\n"
                      "        Item {\n"
                      "            Layout.columnSpan: 4\n"
                      "            Layout.fillWidth: true\n"
                      "            Layout.preferredHeight: 8\n"
                      "\n"
                      "            Rectangle {\n"
                      "                anchors.left: parent.left\n"
                      "                anchors.right: parent.right\n"
                      "                anchors.verticalCenter: parent.verticalCenter\n"
                      "                height: 1\n"
                      "                color: '#808080'\n"
                      "                opacity: 0.45\n"
                      "            }\n"
                      "        }\n";
        }
    }

    stream << "        Label {\n"
              "            Layout.columnSpan: 4\n"
              "            Layout.fillWidth: true\n"
              "            visible: propertyNames.length === 0\n"
              "            textFormat: Text.PlainText\n"
              "            text: qsTr('No properties were discovered for this filter service.')\n"
              "            wrapMode: Text.Wrap\n"
              "        }\n"
              "    }\n"
              "\n"
              "    Connections {\n"
              "        target: filter\n"
              "\n"
              "        function onChanged() {\n"
              "            if (!root.rectangleProperty || root.rectangleChangeGuard)\n"
              "                return;\n"
              "            root.rectangleChangeGuard = true;\n"
              "            if (root.consumeVuiRotation)\n"
              "                root.consumeVuiRotation();\n"
              "            root.setControls();\n"
              "            root.rectangleChangeGuard = false;\n"
              "        }\n"
              "\n"
              "        function onAnimateInChanged() {\n"
              "            if (root.rectangleKeyframes && root.rewriteRectangleKeyframes)\n"
              "                root.rewriteRectangleKeyframes();\n"
              "            root.setControls();\n"
              "            if (root.rectangleRotationProperty && root.rewriteRotationKeyframes)\n"
              "                root.rewriteRotationKeyframes();\n"
              "        }\n"
              "\n"
              "        function onAnimateOutChanged() {\n"
              "            if (root.rectangleKeyframes && root.rewriteRectangleKeyframes)\n"
              "                root.rewriteRectangleKeyframes();\n"
              "            root.setControls();\n"
              "            if (root.rectangleRotationProperty && root.rewriteRotationKeyframes)\n"
              "                root.rewriteRotationKeyframes();\n"
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

    QString vuiError;
    if (!writeRectangleVui(descriptor, outputDir, &vuiError)) {
        file.remove();
        if (errorMessage)
            *errorMessage = vuiError;
        return false;
    }

    if (errorMessage)
        errorMessage->clear();
    return true;
}

static QString addOnKeywordsForMeta(const AddOnFilterDescriptor &descriptor)
{
    QStringList keywords;
    keywords << descriptor.service << QStringLiteral("#addon");

    const bool supportsRgba = descriptor.imageFormats.contains(QStringLiteral("rgba"))
                              || descriptor.imageFormats.contains(QStringLiteral("rgba64"));
    const bool supportsYuv = descriptor.imageFormats.contains(QStringLiteral("yuv422"))
                             || descriptor.imageFormats.contains(QStringLiteral("yuv420p"))
                             || descriptor.imageFormats.contains(QStringLiteral("yuv422p16"))
                             || descriptor.imageFormats.contains(QStringLiteral("yuv420p10"))
                             || descriptor.imageFormats.contains(QStringLiteral("yuv444p10"));
    const bool supportsTenBit = descriptor.imageFormats.contains(QStringLiteral("rgba64"))
                                || descriptor.imageFormats.contains(QStringLiteral("yuv422p16"))
                                || descriptor.imageFormats.contains(QStringLiteral("yuv420p10"))
                                || descriptor.imageFormats.contains(QStringLiteral("yuv444p10"));

    if (supportsRgba)
        keywords << QStringLiteral("#rgba");
    if (supportsYuv)
        keywords << QStringLiteral("#yuv");
    if (supportsTenBit)
        keywords << QStringLiteral("#10bit");

    return keywords.join(QLatin1Char(' '));
}

bool AddOnQmlGenerator::generateMetaQml(const AddOnFilterDescriptor &descriptor,
                                        const QDir &outputDir,
                                        const QString &uiQmlFileName,
                                        const QString &metaQmlFileName,
                                        QString *errorMessage) const
{
    if (descriptor.service.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Missing add-on service identifier");
        return false;
    }

    QFile file(outputDir.filePath(metaQmlFileName));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to open generated add-on meta.qml for writing");
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << "import QtQuick\n"
              "import org.shotcut.qml\n\n"
              "Metadata {\n"
              "    type: Metadata.Filter\n"
              "    objectName: "
           << quotedJsString(QStringLiteral("addOn.%1").arg(descriptor.service))
           << "\n"
              "    name: qsTr("
           << quotedJsString(descriptor.title)
           << ")\n"
              "    keywords: "
           << quotedJsString(addOnKeywordsForMeta(descriptor))
           << "\n"
              "    mlt_service: "
           << quotedJsString(descriptor.service) << "\n";

    if (descriptor.isAudio)
        stream << "    isAudio: true\n";

    stream << "    qml: " << quotedJsString(uiQmlFileName) << "\n";
    if (rectangleVuiParameter(descriptor))
        stream << "    vui: " << quotedJsString(QStringLiteral("vui.qml")) << "\n";

    QStringList keyframeableProperties;
    QStringList simpleProperties;
    bool sawFullRect = false;
    for (const auto &parameter : descriptor.parameters) {
        const bool fullRect = isRectangleVuiParameter(descriptor, parameter);
        bool extraFullRect = false;
        if (fullRect) {
            extraFullRect = sawFullRect;
            sawFullRect = true;
        }
        if (!parameter.supportsKeyframes)
            continue;
        keyframeableProperties << parameter.name;
        if (!extraFullRect)
            simpleProperties << parameter.name;
    }

    if (!keyframeableProperties.isEmpty()) {
        QStringList quotedProps;
        for (const auto &prop : simpleProperties)
            quotedProps << quotedJsString(prop);

        stream << "\n"
                  "    keyframes {\n"
                  "        allowAnimateIn: true\n"
                  "        allowAnimateOut: true\n"
                  "        simpleProperties: ["
               << quotedProps.join(QStringLiteral(", "))
               << "]\n"
                  "        parameters: [\n";

        bool first = true;
        for (const auto &parameter : descriptor.parameters) {
            if (!parameter.supportsKeyframes)
                continue;
            if (!first)
                stream << ",\n";
            first = false;

            const QString paramType = parameter.type.trimmed().toLower();
            const bool isNumericType = paramType == QStringLiteral("integer")
                                       || paramType == QStringLiteral("float");
            const bool isColorType = paramType == QStringLiteral("color");
            const bool isRectangleParameter = paramType == QStringLiteral("rect")
                                              && !isPointOrSizeWidget(parameter);
            const QString displayName = parameter.title.isEmpty() ? parameter.name
                                                                  : parameter.title;

            stream << "            Parameter {\n"
                   << "                name: qsTr(" << quotedJsString(displayName) << ")\n"
                   << "                property: " << quotedJsString(parameter.name) << "\n";

            if (isNumericType) {
                bool okMin = false, okMax = false;
                const double minValue = parameter.minimum.toDouble(&okMin);
                const double maxValue = parameter.maximum.toDouble(&okMax);
                stream << "                isCurve: true\n"
                       << "                minimum: " << (okMin ? minValue : 0.0) << "\n"
                       << "                maximum: " << (okMax ? maxValue : 100.0) << "\n";
            } else if (isColorType) {
                stream << "                isCurve: false\n"
                          "                isColor: true\n";
            } else if (isRectangleParameter) {
                stream << "                isRectangle: true\n";
            }

            if (!parameter.unit.isEmpty())
                stream << "                units: " << quotedJsString(parameter.unit) << "\n";

            stream << "            }";
        }

        stream << "\n"
                  "        ]\n"
                  "    }\n";
    }

    stream << "}\n";

    file.close();
    if (errorMessage)
        errorMessage->clear();
    return true;
}
