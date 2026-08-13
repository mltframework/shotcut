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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Shotcut.Controls as Shotcut

Shotcut.KeyframableFilter {
    id: root
    signal metadataHelpRequested(string service)
    property var propertyNames: ['0', '1', '2', '3', '4', '5', '6', '7']

    property var propertyTitles: {
        '0': qsTr('Amplitude X'),
        '1': qsTr('Amplitude Y'),
        '2': qsTr('Rotation'),
        '3': qsTr('Zoom'),
        '4': qsTr('Speed'),
        '5': qsTr('Opacity'),
        '6': qsTr('Blur'),
        '7': qsTr('Background color')
    }

    property var propertyDefaults: {
        '0': '0.1',
        '1': '0.1',
        '2': '0',
        '3': '0.04',
        '4': '0.05',
        '5': '1',
        '6': '0',
        '7': '#000000'
    }

    property var propertyTypes: {
        '0': 'float',
        '1': 'float',
        '2': 'float',
        '3': 'float',
        '4': 'float',
        '5': 'float',
        '6': 'float',
        '7': 'color'
    }

    property var propertyWidgets: {
        '0': 'spinner',
        '1': 'spinner',
        '2': 'spinner',
        '3': 'spinner',
        '4': 'spinner',
        '5': 'spinner',
        '6': 'spinner',
        '7': 'color'
    }

    property var propertyUnits: ({})

    property var propertyMinimums: {
        '0': '0',
        '1': '0',
        '2': '0',
        '3': '0',
        '4': '0',
        '5': '0',
        '6': '0'
    }

    property var propertyMaximums: {
        '0': '1',
        '1': '1',
        '2': '1',
        '3': '1',
        '4': '1',
        '5': '1',
        '6': '1'
    }

    property var propertyDescriptions: {
        '0': qsTr('Maximum horizontal shake amplitude'),
        '1': qsTr('Maximum vertical shake amplitude'),
        '2': qsTr('Maximum rotation shake'),
        '3': qsTr('Zoom factor to hide black borders'),
        '4': qsTr('Speed or frequency of the shake'),
        '5': qsTr('Opacity of the effect'),
        '6': qsTr('Amount of motion blur'),
        '7': qsTr('Background color for exposed borders')
    }

    property var propertyValues: ({})

    property var propertyNormalizedCoordinates: ({})

    property var propertyNormalizedDefault: ({})

    keyframableParameters: ['0', '1', '2', '3', '4', '5', '6', '7']
    startValues: []
    middleValues: []
    endValues: []

    property var propertyKeyframes: {
        '0': true,
        '1': true,
        '2': true,
        '3': true,
        '4': true,
        '5': true,
        '6': true,
        '7': true
    }

    property string filterService: 'frei0r.camerashake'

    property string filterDescription: 'Camera movement effect with rotation, opacity, and blur.'

    function isKeyframableProperty(name) {
        return !!(root.propertyKeyframes && root.propertyKeyframes[name]);
    }

    function propertyType(name) {
        if (!root.propertyTypes)
            return '';
        var type = root.propertyTypes[name];
        return (type !== undefined && type !== null) ? String(type).toLowerCase() : '';
    }

    function isNumericType(type) {
        return type === 'integer' || type === 'float';
    }

    function propertyWidget(name) {
        if (!root.propertyWidgets)
            return '';
        var widget = root.propertyWidgets[name];
        return (widget !== undefined && widget !== null) ? String(widget).toLowerCase() : '';
    }

    function isIntegerType(type) {
        return type === 'integer';
    }

    function numericSuffix(name) {
        if (!root.propertyUnits)
            return '';
        var unit = root.propertyUnits[name];
        if (unit === undefined || unit === null || unit === '')
            return '';
        unit = String(unit);
        return unit.startsWith(' ') ? unit : (' ' + unit);
    }

    function isBooleanType(type) {
        return type === 'boolean';
    }

    function isColorType(type) {
        return type === 'color';
    }

    function colorValue(name) {
        var value = filter.get(name);
        if (value === undefined || value === null || value === '')
            value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;
        return (value !== undefined && value !== null && value !== '') ? value : '#ffffffff';
    }

    function defaultColorValue(name) {
        var value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;
        return (value !== undefined && value !== null && value !== '') ? value : '#ffffffff';
    }

    function booleanValue(name) {
        var value = filter.get(name);
        if (value === undefined || value === null || value === '')
            value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;
        if (value === undefined || value === null)
            return false;
        if (value === true || value === 1)
            return true;
        var text = String(value).toLowerCase();
        return text === '1' || text === 'true';
    }

    function defaultBooleanValue(name) {
        var value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;
        if (value === undefined || value === null)
            return false;
        if (value === true || value === 1)
            return true;
        var text = String(value).toLowerCase();
        return text === '1' || text === 'true';
    }

    function textValue(name) {
        var value = filter.get(name);
        if (value !== undefined && value !== null && value !== '')
            return value;
        var d = root.propertyDefaults ? root.propertyDefaults[name] : undefined;
        return (d !== undefined && d !== null) ? d : '';
    }

    function defaultTextValue(name) {
        var value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;
        return (value !== undefined && value !== null) ? value : '';
    }

    function numericBound(name, useMin) {
        var map = useMin ? root.propertyMinimums : root.propertyMaximums;
        if (!map)
            return useMin ? 0 : 100;
        var raw = map[name];
        if (raw === undefined || raw === null || raw === '')
            return useMin ? 0 : 100;
        var parsed = Number(raw);
        if (isNaN(parsed))
            return useMin ? 0 : 100;
        return parsed;
    }

    function numericValue(name, type) {
        var value;
        if (root.isKeyframableProperty(name))
            value = filter.getDouble(name, root.getPosition());
        else
            value = filter.get(name);
        if (value === undefined || value === null || value === '')
            value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;
        var parsed = Number(value);
        if (isNaN(parsed)) {
            parsed = numericBound(name, true);
            var maxBound = numericBound(name, false);
            if (parsed > maxBound)
                parsed = maxBound;
        }
        if (isIntegerType(type))
            parsed = Math.round(parsed);
        return parsed;
    }

    function defaultNumericValue(name, type) {
        var value = root.propertyDefaults ? root.propertyDefaults[name] : undefined;
        var parsed = Number(value);
        if (isNaN(parsed)) {
            parsed = numericBound(name, true);
            var maxBound = numericBound(name, false);
            if (parsed > maxBound)
                parsed = maxBound;
        }
        if (isIntegerType(type))
            parsed = Math.round(parsed);
        return parsed;
    }

    function setControls() {
        blockUpdate = true;
        param_0_editor.value = root.numericValue('0', root.propertyType('0'));
        param_0_keyframes.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('0') > 0;
        param_1_editor.value = root.numericValue('1', root.propertyType('1'));
        param_1_keyframes.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('1') > 0;
        param_2_editor.value = root.numericValue('2', root.propertyType('2'));
        param_2_keyframes.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('2') > 0;
        param_3_editor.value = root.numericValue('3', root.propertyType('3'));
        param_3_keyframes.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('3') > 0;
        param_4_editor.value = root.numericValue('4', root.propertyType('4'));
        param_4_keyframes.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('4') > 0;
        param_5_editor.value = root.numericValue('5', root.propertyType('5'));
        param_5_keyframes.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('5') > 0;
        param_6_editor.value = root.numericValue('6', root.propertyType('6'));
        param_6_keyframes.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('6') > 0;
        param_7_editor.value = root.colorValue('7');
        param_7_keyframes.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('7') > 0;
        blockUpdate = false;
    }

    width: 360
    height: gridLayout.implicitHeight + 16

    Component.onCompleted: {
        if (filter.isNew) {
            for (var i = 0; i < propertyNames.length; ++i) {
                var p = propertyNames[i];
                var d = root.propertyDefaults[p];
                var type = propertyType(p);
                var widget = propertyWidget(p);
                if (d !== undefined && d !== null && d !== '') {
                    if (type === 'rect' && root.propertyNormalizedDefault && root.propertyNormalizedDefault[p] === 'yes') {
                        var _dp = String(d).trim().split(/\s+/);
                        var _dx = _dp.length > 0 ? parseFloat(_dp[0]) : 0;
                        var _dy = _dp.length > 1 ? parseFloat(_dp[1]) : 0;
                        if (isNaN(_dx))
                            _dx = 0;
                        if (isNaN(_dy))
                            _dy = 0;
                        _dx *= profile.width;
                        _dy *= profile.height;
                        filter.set(p, _dx + ' ' + _dy + ' 0 0 0');
                    } else {
                        filter.set(p, isBooleanType(type) ? (booleanValue(p) ? '1' : '0') : ((isNumericType(type) && widget === 'text') ? String(d) : (isNumericType(type) ? Number(d) : d)));
                    }
                }
            }
            filter.savePreset(propertyNames);
        }
        setControls();
        if (keyframableParameters.length > 0)
            initializeSimpleKeyframes();
    }

    GridLayout {
        id: gridLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        columns: 4
        columnSpacing: 8
        rowSpacing: 6

        Label {
            text: qsTr('Preset')
            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignRight
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        }

        Shotcut.Preset {
            id: preset
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            parameters: root.propertyNames ? root.propertyNames.slice(0) : []
            onBeforePresetLoaded: {
                filter.resetProperty('shotcut:animIn');
                filter.resetProperty('shotcut:animOut');
                if (keyframableParameters.length > 0)
                    resetSimpleKeyframes();
                for (var i = 0; i < root.propertyNames.length; ++i)
                    filter.resetProperty(root.propertyNames[i]);
            }
            onPresetSelected: {
                filter.animateIn = Math.round(filter.getDouble('shotcut:animIn'));
                filter.animateOut = Math.round(filter.getDouble('shotcut:animOut'));
                root.setControls();
                if (keyframableParameters.length > 0)
                    initializeSimpleKeyframes();
            }
        }

        Label {
            text: (root.propertyTitles && root.propertyTitles['0']) ? root.propertyTitles['0'] : '0'
            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignRight
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            elide: Text.ElideRight
            ToolTip.delay: 400
            ToolTip.visible: param_0_hover.containsMouse
            ToolTip.text: {
                if (root.propertyDescriptions && root.propertyDescriptions['0'])
                    return root.propertyDescriptions['0'];
                return text;
            }

            MouseArea {
                id: param_0_hover
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }
        }

        Shotcut.SliderSpinner {
            id: param_0_editor
            Layout.columnSpan: 1
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.minimumWidth: 180
            readonly property string propertyName: '0'
            readonly property string typeName: root.propertyType(propertyName)
            value: root.numericValue(propertyName, typeName)
            minimumValue: Math.min(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            maximumValue: Math.max(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            decimals: root.isIntegerType(typeName) ? 0 : 3
            suffix: root.numericSuffix(propertyName)
            onValueChanged: {
                if (root.isKeyframableProperty(propertyName)) {
                    root.updateFilter(propertyName, value, param_0_keyframes, root.getPosition());
                } else {
                    var current = Number(filter.get(propertyName));
                    if (isNaN(current) || current !== value)
                        filter.set(propertyName, value);
                }
            }
        }

        Shotcut.UndoButton {
            readonly property string propertyName: '0'
            readonly property string typeName: root.propertyType(propertyName)
            onClicked: {
                var defaultValue = root.defaultNumericValue(propertyName, typeName);
                if (root.isKeyframableProperty(propertyName))
                    root.updateFilter(propertyName, defaultValue, param_0_keyframes, root.getPosition());
                else
                    filter.set(propertyName, defaultValue);
                root.setControls();
            }
        }

        Shotcut.KeyframesButton {
            id: param_0_keyframes
            readonly property string propertyName: '0'
            visible: root.isKeyframableProperty(propertyName)
            Layout.preferredWidth: visible ? implicitWidth : 0
            onToggled: {
                root.toggleKeyframes(checked, propertyName, param_0_editor.value);
            }
        }

        Label {
            text: (root.propertyTitles && root.propertyTitles['1']) ? root.propertyTitles['1'] : '1'
            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignRight
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            elide: Text.ElideRight
            ToolTip.delay: 400
            ToolTip.visible: param_1_hover.containsMouse
            ToolTip.text: {
                if (root.propertyDescriptions && root.propertyDescriptions['1'])
                    return root.propertyDescriptions['1'];
                return text;
            }

            MouseArea {
                id: param_1_hover
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }
        }

        Shotcut.SliderSpinner {
            id: param_1_editor
            Layout.columnSpan: 1
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.minimumWidth: 180
            readonly property string propertyName: '1'
            readonly property string typeName: root.propertyType(propertyName)
            value: root.numericValue(propertyName, typeName)
            minimumValue: Math.min(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            maximumValue: Math.max(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            decimals: root.isIntegerType(typeName) ? 0 : 3
            suffix: root.numericSuffix(propertyName)
            onValueChanged: {
                if (root.isKeyframableProperty(propertyName)) {
                    root.updateFilter(propertyName, value, param_1_keyframes, root.getPosition());
                } else {
                    var current = Number(filter.get(propertyName));
                    if (isNaN(current) || current !== value)
                        filter.set(propertyName, value);
                }
            }
        }

        Shotcut.UndoButton {
            readonly property string propertyName: '1'
            readonly property string typeName: root.propertyType(propertyName)
            onClicked: {
                var defaultValue = root.defaultNumericValue(propertyName, typeName);
                if (root.isKeyframableProperty(propertyName))
                    root.updateFilter(propertyName, defaultValue, param_1_keyframes, root.getPosition());
                else
                    filter.set(propertyName, defaultValue);
                root.setControls();
            }
        }

        Shotcut.KeyframesButton {
            id: param_1_keyframes
            readonly property string propertyName: '1'
            visible: root.isKeyframableProperty(propertyName)
            Layout.preferredWidth: visible ? implicitWidth : 0
            onToggled: {
                root.toggleKeyframes(checked, propertyName, param_1_editor.value);
            }
        }

        Label {
            text: (root.propertyTitles && root.propertyTitles['2']) ? root.propertyTitles['2'] : '2'
            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignRight
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            elide: Text.ElideRight
            ToolTip.delay: 400
            ToolTip.visible: param_2_hover.containsMouse
            ToolTip.text: {
                if (root.propertyDescriptions && root.propertyDescriptions['2'])
                    return root.propertyDescriptions['2'];
                return text;
            }

            MouseArea {
                id: param_2_hover
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }
        }

        Shotcut.SliderSpinner {
            id: param_2_editor
            Layout.columnSpan: 1
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.minimumWidth: 180
            readonly property string propertyName: '2'
            readonly property string typeName: root.propertyType(propertyName)
            value: root.numericValue(propertyName, typeName)
            minimumValue: Math.min(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            maximumValue: Math.max(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            decimals: root.isIntegerType(typeName) ? 0 : 3
            suffix: root.numericSuffix(propertyName)
            onValueChanged: {
                if (root.isKeyframableProperty(propertyName)) {
                    root.updateFilter(propertyName, value, param_2_keyframes, root.getPosition());
                } else {
                    var current = Number(filter.get(propertyName));
                    if (isNaN(current) || current !== value)
                        filter.set(propertyName, value);
                }
            }
        }

        Shotcut.UndoButton {
            readonly property string propertyName: '2'
            readonly property string typeName: root.propertyType(propertyName)
            onClicked: {
                var defaultValue = root.defaultNumericValue(propertyName, typeName);
                if (root.isKeyframableProperty(propertyName))
                    root.updateFilter(propertyName, defaultValue, param_2_keyframes, root.getPosition());
                else
                    filter.set(propertyName, defaultValue);
                root.setControls();
            }
        }

        Shotcut.KeyframesButton {
            id: param_2_keyframes
            readonly property string propertyName: '2'
            visible: root.isKeyframableProperty(propertyName)
            Layout.preferredWidth: visible ? implicitWidth : 0
            onToggled: {
                root.toggleKeyframes(checked, propertyName, param_2_editor.value);
            }
        }

        Label {
            text: (root.propertyTitles && root.propertyTitles['3']) ? root.propertyTitles['3'] : '3'
            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignRight
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            elide: Text.ElideRight
            ToolTip.delay: 400
            ToolTip.visible: param_3_hover.containsMouse
            ToolTip.text: {
                if (root.propertyDescriptions && root.propertyDescriptions['3'])
                    return root.propertyDescriptions['3'];
                return text;
            }

            MouseArea {
                id: param_3_hover
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }
        }

        Shotcut.SliderSpinner {
            id: param_3_editor
            Layout.columnSpan: 1
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.minimumWidth: 180
            readonly property string propertyName: '3'
            readonly property string typeName: root.propertyType(propertyName)
            value: root.numericValue(propertyName, typeName)
            minimumValue: Math.min(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            maximumValue: Math.max(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            decimals: root.isIntegerType(typeName) ? 0 : 3
            suffix: root.numericSuffix(propertyName)
            onValueChanged: {
                if (root.isKeyframableProperty(propertyName)) {
                    root.updateFilter(propertyName, value, param_3_keyframes, root.getPosition());
                } else {
                    var current = Number(filter.get(propertyName));
                    if (isNaN(current) || current !== value)
                        filter.set(propertyName, value);
                }
            }
        }

        Shotcut.UndoButton {
            readonly property string propertyName: '3'
            readonly property string typeName: root.propertyType(propertyName)
            onClicked: {
                var defaultValue = root.defaultNumericValue(propertyName, typeName);
                if (root.isKeyframableProperty(propertyName))
                    root.updateFilter(propertyName, defaultValue, param_3_keyframes, root.getPosition());
                else
                    filter.set(propertyName, defaultValue);
                root.setControls();
            }
        }

        Shotcut.KeyframesButton {
            id: param_3_keyframes
            readonly property string propertyName: '3'
            visible: root.isKeyframableProperty(propertyName)
            Layout.preferredWidth: visible ? implicitWidth : 0
            onToggled: {
                root.toggleKeyframes(checked, propertyName, param_3_editor.value);
            }
        }

        Label {
            text: (root.propertyTitles && root.propertyTitles['4']) ? root.propertyTitles['4'] : '4'
            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignRight
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            elide: Text.ElideRight
            ToolTip.delay: 400
            ToolTip.visible: param_4_hover.containsMouse
            ToolTip.text: {
                if (root.propertyDescriptions && root.propertyDescriptions['4'])
                    return root.propertyDescriptions['4'];
                return text;
            }

            MouseArea {
                id: param_4_hover
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }
        }

        Shotcut.SliderSpinner {
            id: param_4_editor
            Layout.columnSpan: 1
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.minimumWidth: 180
            readonly property string propertyName: '4'
            readonly property string typeName: root.propertyType(propertyName)
            value: root.numericValue(propertyName, typeName)
            minimumValue: Math.min(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            maximumValue: Math.max(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            decimals: root.isIntegerType(typeName) ? 0 : 3
            suffix: root.numericSuffix(propertyName)
            onValueChanged: {
                if (root.isKeyframableProperty(propertyName)) {
                    root.updateFilter(propertyName, value, param_4_keyframes, root.getPosition());
                } else {
                    var current = Number(filter.get(propertyName));
                    if (isNaN(current) || current !== value)
                        filter.set(propertyName, value);
                }
            }
        }

        Shotcut.UndoButton {
            readonly property string propertyName: '4'
            readonly property string typeName: root.propertyType(propertyName)
            onClicked: {
                var defaultValue = root.defaultNumericValue(propertyName, typeName);
                if (root.isKeyframableProperty(propertyName))
                    root.updateFilter(propertyName, defaultValue, param_4_keyframes, root.getPosition());
                else
                    filter.set(propertyName, defaultValue);
                root.setControls();
            }
        }

        Shotcut.KeyframesButton {
            id: param_4_keyframes
            readonly property string propertyName: '4'
            visible: root.isKeyframableProperty(propertyName)
            Layout.preferredWidth: visible ? implicitWidth : 0
            onToggled: {
                root.toggleKeyframes(checked, propertyName, param_4_editor.value);
            }
        }

        Label {
            text: (root.propertyTitles && root.propertyTitles['5']) ? root.propertyTitles['5'] : '5'
            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignRight
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            elide: Text.ElideRight
            ToolTip.delay: 400
            ToolTip.visible: param_5_hover.containsMouse
            ToolTip.text: {
                if (root.propertyDescriptions && root.propertyDescriptions['5'])
                    return root.propertyDescriptions['5'];
                return text;
            }

            MouseArea {
                id: param_5_hover
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }
        }

        Shotcut.SliderSpinner {
            id: param_5_editor
            Layout.columnSpan: 1
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.minimumWidth: 180
            readonly property string propertyName: '5'
            readonly property string typeName: root.propertyType(propertyName)
            value: root.numericValue(propertyName, typeName)
            minimumValue: Math.min(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            maximumValue: Math.max(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            decimals: root.isIntegerType(typeName) ? 0 : 3
            suffix: root.numericSuffix(propertyName)
            onValueChanged: {
                if (root.isKeyframableProperty(propertyName)) {
                    root.updateFilter(propertyName, value, param_5_keyframes, root.getPosition());
                } else {
                    var current = Number(filter.get(propertyName));
                    if (isNaN(current) || current !== value)
                        filter.set(propertyName, value);
                }
            }
        }

        Shotcut.UndoButton {
            readonly property string propertyName: '5'
            readonly property string typeName: root.propertyType(propertyName)
            onClicked: {
                var defaultValue = root.defaultNumericValue(propertyName, typeName);
                if (root.isKeyframableProperty(propertyName))
                    root.updateFilter(propertyName, defaultValue, param_5_keyframes, root.getPosition());
                else
                    filter.set(propertyName, defaultValue);
                root.setControls();
            }
        }

        Shotcut.KeyframesButton {
            id: param_5_keyframes
            readonly property string propertyName: '5'
            visible: root.isKeyframableProperty(propertyName)
            Layout.preferredWidth: visible ? implicitWidth : 0
            onToggled: {
                root.toggleKeyframes(checked, propertyName, param_5_editor.value);
            }
        }

        Label {
            text: (root.propertyTitles && root.propertyTitles['6']) ? root.propertyTitles['6'] : '6'
            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignRight
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            elide: Text.ElideRight
            ToolTip.delay: 400
            ToolTip.visible: param_6_hover.containsMouse
            ToolTip.text: {
                if (root.propertyDescriptions && root.propertyDescriptions['6'])
                    return root.propertyDescriptions['6'];
                return text;
            }

            MouseArea {
                id: param_6_hover
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }
        }

        Shotcut.SliderSpinner {
            id: param_6_editor
            Layout.columnSpan: 1
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.minimumWidth: 180
            readonly property string propertyName: '6'
            readonly property string typeName: root.propertyType(propertyName)
            value: root.numericValue(propertyName, typeName)
            minimumValue: Math.min(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            maximumValue: Math.max(root.numericBound(propertyName, true), root.numericBound(propertyName, false))
            decimals: root.isIntegerType(typeName) ? 0 : 3
            suffix: root.numericSuffix(propertyName)
            onValueChanged: {
                if (root.isKeyframableProperty(propertyName)) {
                    root.updateFilter(propertyName, value, param_6_keyframes, root.getPosition());
                } else {
                    var current = Number(filter.get(propertyName));
                    if (isNaN(current) || current !== value)
                        filter.set(propertyName, value);
                }
            }
        }

        Shotcut.UndoButton {
            readonly property string propertyName: '6'
            readonly property string typeName: root.propertyType(propertyName)
            onClicked: {
                var defaultValue = root.defaultNumericValue(propertyName, typeName);
                if (root.isKeyframableProperty(propertyName))
                    root.updateFilter(propertyName, defaultValue, param_6_keyframes, root.getPosition());
                else
                    filter.set(propertyName, defaultValue);
                root.setControls();
            }
        }

        Shotcut.KeyframesButton {
            id: param_6_keyframes
            readonly property string propertyName: '6'
            visible: root.isKeyframableProperty(propertyName)
            Layout.preferredWidth: visible ? implicitWidth : 0
            onToggled: {
                root.toggleKeyframes(checked, propertyName, param_6_editor.value);
            }
        }

        Label {
            text: (root.propertyTitles && root.propertyTitles['7']) ? root.propertyTitles['7'] : '7'
            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignRight
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            elide: Text.ElideRight
            ToolTip.delay: 400
            ToolTip.visible: param_7_hover.containsMouse
            ToolTip.text: {
                if (root.propertyDescriptions && root.propertyDescriptions['7'])
                    return root.propertyDescriptions['7'];
                return text;
            }

            MouseArea {
                id: param_7_hover
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }
        }

        Shotcut.ColorPicker {
            id: param_7_editor
            Layout.columnSpan: 1
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            readonly property string propertyName: '7'
            readonly property string typeName: root.propertyType(propertyName)
            property bool isReady: false
            value: root.colorValue(propertyName)
            alpha: true
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (!isReady)
                    return;
                if (root.isKeyframableProperty(propertyName)) {
                    root.updateFilter(propertyName, Qt.color(value), param_7_keyframes, root.getPosition());
                } else {
                    var current = filter.get(propertyName);
                    if (String(current) !== String(value))
                        filter.set(propertyName, value);
                }
            }
        }

        Shotcut.UndoButton {
            readonly property string propertyName: '7'
            readonly property string typeName: root.propertyType(propertyName)
            onClicked: {
                var defaultValue = root.defaultColorValue(propertyName);
                if (root.isKeyframableProperty(propertyName))
                    root.updateFilter(propertyName, Qt.color(defaultValue), param_7_keyframes, root.getPosition());
                else
                    filter.set(propertyName, defaultValue);
                root.setControls();
            }
        }

        Shotcut.KeyframesButton {
            id: param_7_keyframes
            readonly property string propertyName: '7'
            visible: root.isKeyframableProperty(propertyName)
            Layout.preferredWidth: visible ? implicitWidth : 0
            onToggled: {
                root.toggleKeyframes(checked, propertyName, Qt.color(param_7_editor.value));
            }
        }

        Label {
            Layout.columnSpan: 4
            Layout.fillWidth: true
            visible: propertyNames.length === 0
            textFormat: Text.PlainText
            text: qsTr('No properties were discovered for this filter service.')
            wrapMode: Text.Wrap
        }
    }

    Connections {
        target: filter

        function onAnimateInChanged() {
            root.setControls();
        }

        function onAnimateOutChanged() {
            root.setControls();
        }
    }

    Connections {
        target: producer

        function onPositionChanged() {
            root.setControls();
        }
    }
}
