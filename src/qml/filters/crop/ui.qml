/*
 * Copyright (c) 2014-2015 Meltytech, LLC
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

Item {
    property var defaultParameters: ['left', 'right', 'top', 'bottom', 'center', 'center_bias']
    width: 350
    height: 200
    
    function setEnabled() {
        if (filter.get('center') == 1) {
            biasslider.enabled = true
            biasundo.enabled = true
            topslider.enabled = false
            topundo.enabled = false
            bottomslider.enabled = false
            bottomundo.enabled = false
            leftslider.enabled = false
            leftundo.enabled = false
            rightslider.enabled = false
            rightundo.enabled = false
        } else {
            biasslider.enabled = false
            biasundo.enabled = false
            topslider.enabled = true
            topundo.enabled = true
            bottomslider.enabled = true
            bottomundo.enabled = true
            leftslider.enabled = true
            leftundo.enabled = true
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

            biasslider.value = +filter.get('center_bias')
            topslider.value = +filter.get('top')
            bottomslider.value = +filter.get('bottom')
            leftslider.value = +filter.get('left')
            rightslider.value = +filter.get('right')
        }
        centerCheckBox.checked = filter.get('center') == '1'
        setEnabled()
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            Layout.columnSpan: 2
            parameters: defaultParameters
            onPresetSelected: {
                centerCheckBox.checked = filter.get('center') == '1'
                biasslider.value = +filter.get('center_bias')
                topslider.value = +filter.get('top')
                bottomslider.value = +filter.get('bottom')
                leftslider.value = +filter.get('left')
                rightslider.value = +filter.get('right')
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
            Layout.fillWidth: true;
        }
        UndoButton {
            onClicked: {
                centerCheckBox.checked = false
                filter.set('center', false)
                setEnabled()
            }
        }

        Label {
            text: qsTr('Center bias')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: biasslider
            minimumValue: -Math.max(profile.width, profile.height) / 2
            maximumValue: Math.max(profile.width, profile.height) / 2
            suffix: ' px'
            value: +filter.get('center_bias')
            onValueChanged: filter.set('center_bias', value)
        }
        UndoButton {
            id: biasundo
            onClicked: biasslider.value = 0
        }

        Label {
            text: qsTr('Top')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: topslider
            minimumValue: 0
            maximumValue: profile.height
            suffix: ' px'
            value: +filter.get('top')
            onValueChanged: filter.set('top', value)
        }
        UndoButton {
            id: topundo
            onClicked: topslider.value = 0
        }

        Label {
            text: qsTr('Bottom')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: bottomslider
            minimumValue: 0
            maximumValue: profile.height
            suffix: ' px'
            value: +filter.get('bottom')
            onValueChanged: filter.set('bottom', value)
        }
        UndoButton {
            id: bottomundo
            onClicked: bottomslider.value = 0
        }

        Label {
            text: qsTr('Left')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: leftslider
            minimumValue: 0
            maximumValue: profile.width
            suffix: ' px'
            value: +filter.get('left')
            onValueChanged: filter.set('left', value)
        }
        UndoButton {
            id: leftundo
            onClicked: leftslider.value = 0
        }

        Label {
            text: qsTr('Right')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: rightslider
            minimumValue: 0
            maximumValue: profile.width
            suffix: ' px'
            value: +filter.get('right')
            onValueChanged: filter.set('right', value)
        }
        UndoButton {
            id: rightundo
            onClicked: rightslider.value = 0
        }
        
        Item {
            Layout.fillHeight: true;
        }
    }
}
