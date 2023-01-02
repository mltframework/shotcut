/*
 * Copyright (c) 2014-2021 Meltytech, LLC
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
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    property int producerWidth: (producer.get('meta.media.width') === null) ? profile.width : producer.get('meta.media.width')
    property int producerHeight: (producer.get('meta.media.height') === null) ? profile.height : producer.get('meta.media.height')
    property double widthScaleFactor: profile.width / producerWidth
    property double heightScaleFactor: profile.height / producerHeight
    property var defaultParameters: ['left', 'right', 'top', 'bottom', 'center', 'center_bias']

    function setEnabled() {
        if (filter.get('center') === '1') {
            biasslider.enabled = true;
            biasundo.enabled = true;
            topslider.enabled = false;
            topundo.enabled = false;
            bottomslider.enabled = false;
            bottomundo.enabled = false;
            leftslider.enabled = false;
            leftundo.enabled = false;
            rightslider.enabled = false;
            rightundo.enabled = false;
        } else {
            biasslider.enabled = false;
            biasundo.enabled = false;
            topslider.enabled = true;
            topundo.enabled = true;
            bottomslider.enabled = true;
            bottomundo.enabled = true;
            leftslider.enabled = true;
            leftundo.enabled = true;
            rightslider.enabled = true;
            rightundo.enabled = true;
        }
    }

    function setControls() {
        centerCheckBox.checked = filter.get('center') === '1';
        biasslider.value = filter.get('center_bias');
        topslider.value = filter.get('top');
        bottomslider.value = filter.get('bottom');
        leftslider.value = filter.get('left');
        rightslider.value = filter.get('right');
    }

    width: 350
    height: 200
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('use_profile', '1');
            filter.set("center", 0);
            filter.set("center_bias", 0);
            filter.set("top", 0);
            filter.set("bottom", 0);
            filter.set("left", 0);
            filter.set("right", 0);
            centerCheckBox.checked = false;
            filter.savePreset(defaultParameters);
        } else {
            if (filter.get('use_profile') !== '1') {
                filter.blockSignals = true;
                filter.set('use_profile', '1');
                filter.set('top', Math.round(filter.getDouble('top') * heightScaleFactor));
                filter.set('bottom', Math.round(filter.getDouble('bottom') * heightScaleFactor));
                filter.set('left', Math.round(filter.getDouble('left') * widthScaleFactor));
                filter.blockSignals = false;
                filter.set('right', Math.round(filter.getDouble('right') * widthScaleFactor));
            }
        }
        setControls();
        setEnabled();
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            Layout.columnSpan: 2
            parameters: defaultParameters
            onPresetSelected: {
                setControls();
                setEnabled();
            }
        }

        CheckBox {
            id: centerCheckBox

            property bool isReady: false

            text: qsTr('Center')
            Component.onCompleted: isReady = true
            onClicked: {
                if (isReady) {
                    filter.set('center', checked);
                    setEnabled();
                }
            }
        }

        Item {
            width: 1
        }

        Shotcut.UndoButton {
            onClicked: {
                centerCheckBox.checked = false;
                filter.set('center', false);
                setEnabled();
            }
        }

        Label {
            text: qsTr('Center bias')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: biasslider

            minimumValue: Math.round(-Math.max(profile.width, profile.height) / 2)
            maximumValue: Math.round(Math.max(profile.width, profile.height) / 2)
            suffix: ' px'
            onValueChanged: filter.set('center_bias', value)
        }

        Shotcut.UndoButton {
            id: biasundo

            onClicked: biasslider.value = 0
        }

        Label {
            text: qsTr('Top')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: topslider

            minimumValue: 0
            maximumValue: profile.height
            suffix: ' px'
            onValueChanged: filter.set('top', value)
        }

        Shotcut.UndoButton {
            id: topundo

            onClicked: topslider.value = 0
        }

        Label {
            text: qsTr('Bottom')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: bottomslider

            minimumValue: 0
            maximumValue: profile.height
            suffix: ' px'
            onValueChanged: filter.set('bottom', value)
        }

        Shotcut.UndoButton {
            id: bottomundo

            onClicked: bottomslider.value = 0
        }

        Label {
            text: qsTr('Left')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: leftslider

            minimumValue: 0
            maximumValue: profile.width
            suffix: ' px'
            onValueChanged: filter.set('left', value)
        }

        Shotcut.UndoButton {
            id: leftundo

            onClicked: leftslider.value = 0
        }

        Label {
            text: qsTr('Right')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: rightslider

            minimumValue: 0
            maximumValue: profile.width
            suffix: ' px'
            onValueChanged: filter.set('right', value)
        }

        Shotcut.UndoButton {
            id: rightundo

            onClicked: rightslider.value = 0
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
