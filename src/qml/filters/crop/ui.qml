/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Brian Matherly <pez4brian@yahoo.com>
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

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Rectangle {
    property var defaultParameters: ['left', 'right', 'top', 'bottom', 'center', 'center_bias']
    width: 400
    height: 200
    color: 'transparent'
    
    function setEnabled() {
        if (filter.get('center') == 1) {
            biasslider.enabled = true
            biasspinner.enabled = true
            biasundo.enabled = true
            topspinner.enabled = false
            topslider.enabled = false
            topundo.enabled = false
            bottomspinner.enabled = false
            bottomslider.enabled = false
            bottomundo.enabled = false
            leftspinner.enabled = false
            leftslider.enabled = false
            leftundo.enabled = false
            rightspinner.enabled = false
            rightslider.enabled = false
            rightundo.enabled = false
        } else {
            biasslider.enabled = false
            biasspinner.enabled = false
            biasundo.enabled = false
            topspinner.enabled = true
            topslider.enabled = true
            topundo.enabled = true
            bottomspinner.enabled = true
            bottomslider.enabled = true
            bottomundo.enabled = true
            leftspinner.enabled = true
            leftslider.enabled = true
            leftundo.enabled = true
            rightspinner.enabled = true
            rightslider.enabled = true
            rightundo.enabled = true
        }
    }
    
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set("center", 0);
            filter.set("center_bias", 0);
            filter.set("top", 0);
            filter.set("bottom", 0);
            filter.set("left", 0);
            filter.set("right", 0);
            centerCheckBox.checked = false
            filter.savePreset(defaultParameters)
        }
        centerCheckBox.checked = filter.get('center') == '1'
        biasspinner.value = +filter.get('center_bias')
        topspinner.value = +filter.get('top')
        bottomspinner.value = +filter.get('bottom')
        leftspinner.value = +filter.get('left')
        rightspinner.value = +filter.get('right')
        setEnabled()
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Preset {
            Layout.columnSpan: 4
            parameters: defaultParameters
            onPresetSelected: {
                centerCheckBox.checked = filter.get('center') == '1'
                biasspinner.value = +filter.get('center_bias')
                topspinner.value = +filter.get('top')
                bottomspinner.value = +filter.get('bottom')
                leftspinner.value = +filter.get('left')
                rightspinner.value = +filter.get('right')
                setEnabled()
            }
        }

        CheckBox {
            id: centerCheckBox
            text: qsTr('Center')
            checked: filter.get('center') == '1'
            property bool isReady: false
            Component.onCompleted: isReady = true
            onClicked: {
                if (isReady) {
                    filter.set('center', checked)
                    setEnabled()
                }
            }
        }
        Item {
            Layout.columnSpan: 2
            Layout.fillWidth: true;
        }
        UndoButton {
            onClicked: {
                centerCheckBox.checked = false
                filter.set('center', false)
            }
        }

        Label { text: qsTr('Center Bias') }
        Slider {
            id: biasslider
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            value: +filter.get('center_bias')
            minimumValue: -Math.max(profile.width, profile.height) / 2
            maximumValue: Math.max(profile.width, profile.height) / 2
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    biasspinner.value = value
                    filter.set('center_bias', value)
                }
            }
        }
        SpinBox {
            id: biasspinner
            Layout.minimumWidth: 80
            value: +filter.get('center_bias')
            suffix: ' px'
            minimumValue: -Math.max(profile.width, profile.height) / 2
            maximumValue: Math.max(profile.width, profile.height) / 2
            decimals: 0
            onValueChanged: biasslider.value = value
        }
        UndoButton {
            id: biasundo
            onClicked: biasslider.value = 0
        }

        Label { text: qsTr('Top') }
        Slider {
            id: topslider
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            value: +filter.get('top')
            minimumValue: 0
            maximumValue: profile.height
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    topspinner.value = value
                    filter.set('top', value)
                }
            }
        }
        SpinBox {
            id: topspinner
            Layout.minimumWidth: 80
            value: +filter.get('top')
            suffix: ' px'
            minimumValue: 0
            maximumValue: profile.height
            decimals: 0
            onValueChanged: topslider.value = value
        }
        UndoButton {
            id: topundo
            onClicked: topslider.value = 0
        }

        Label { text: qsTr('Bottom') }
        Slider {
            id: bottomslider
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            value: +filter.get('bottom')
            minimumValue: 0
            maximumValue: profile.height
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    bottomspinner.value = value
                    filter.set('bottom', value)
                }
            }
        }
        SpinBox {
            id: bottomspinner
            Layout.minimumWidth: 80
            value: +filter.get('bottom')
            suffix: ' px'
            minimumValue: 0
            maximumValue: profile.height
            decimals: 0
            onValueChanged: bottomslider.value = value
        }
        UndoButton {
            id: bottomundo
            onClicked: bottomslider.value = 0
        }

        Label { text: qsTr('Left') }
        Slider {
            id: leftslider
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            value: +filter.get('left')
            minimumValue: 0
            maximumValue: profile.width
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    leftspinner.value = value
                    filter.set('left', value)
                }
            }
        }
        SpinBox {
            id: leftspinner
            Layout.minimumWidth: 80
            value: +filter.get('left')
            suffix: ' px'
            minimumValue: 0
            maximumValue: profile.width
            decimals: 0
            onValueChanged: leftslider.value = value
        }
        UndoButton {
            id: leftundo
            onClicked: leftslider.value = 0
        }

        Label { text: qsTr('Right') }
        Slider {
            id: rightslider
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            value: +filter.get('right')
            minimumValue: 0
            maximumValue: profile.width
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    rightspinner.value = value
                    filter.set('right', value)
                }
            }
        }
        SpinBox {
            id: rightspinner
            Layout.minimumWidth: 80
            value: +filter.get('right')
            suffix: ' px'
            minimumValue: 0
            maximumValue: profile.width
            decimals: 0
            onValueChanged: rightslider.value = value
        }
        UndoButton {
            id: rightundo
            onClicked: rightslider.value = 0
        }
        
        Item {
            Layout.fillHeight: true;
            Layout.columnSpan: 4
        }
    }
}
