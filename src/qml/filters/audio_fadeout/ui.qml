/*
 * Copyright (c) 2014-2024 Meltytech, LLC
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

Item {
    property alias duration: timeSpinner.value
    property bool _blockUpdate: false

    function updateFilter() {
        filter.resetProperty('level');
        filter.set('level', 0, Math.max(filter.duration - duration, 0));
        filter.set('level', -60, filter.duration - 1);
        filter.setKeyFrameType('level', 0, curveCombo.currentValue);
    }

    width: 100
    height: 50
    objectName: 'fadeOut'
    Component.onCompleted: {
        _blockUpdate = true;
        if (filter.isNew) {
            duration = Math.ceil(settings.audioOutDuration * profile.fps);
            filter.animateOut = duration;
            curveCombo.setCurrentValue(settings.audioOutCurve);
            updateFilter();
        } else if (filter.animateOut === 0) {
            // Convert legacy filter.
            duration = filter.duration;
            filter.set('in', producer.in);
            filter.set('out', producer.out);
        } else {
            duration = filter.animateOut;
        }
        curveCombo.setCurrentValue(filter.getKeyFrameType('level', 0));
        _blockUpdate = false;
    }

    Connections {
        function onAnimateOutChanged() {
            _blockUpdate = true;
            duration = filter.animateOut;
            _blockUpdate = false;
        }

        target: filter
    }

    GridLayout {
        anchors.fill: parent
        columns: 5

        Label {
            id: durationLabel

            text: qsTr('Duration')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.TimeSpinner {
            id: timeSpinner

            undoButtonVisible: false
            saveButtonVisible: false
            minimumValue: 2
            maximumValue: 5000
            onValueChanged: {
                if (_blockUpdate)
                    return;
                filter.startUndoParameterCommand(durationLabel.text);
                filter.animateOut = duration;
                updateFilter();
                filter.endUndoCommand();
            }
        }

        Shotcut.UndoButton {
            onClicked: duration = Math.ceil(settings.audioOutDuration * profile.fps)
        }

        Shotcut.SaveDefaultButton {
            onClicked: settings.audioOutDuration = duration / profile.fps
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            id: curveLabel

            text: qsTr('Type')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.CurveComboBox {
            id: curveCombo

            implicitContentWidthPolicy: ComboBox.WidestText
            onActivated: {
                if (_blockUpdate)
                    return;
                filter.startUndoParameterCommand(curveLabel.text);
                filter.setKeyFrameType('level', 0, curveCombo.currentValue);
                filter.endUndoCommand();
            }
        }

        Shotcut.UndoButton {
            id: undoButton

            onClicked: curveCombo.setCurrentValue(settings.audioOutCurve)
        }

        Shotcut.SaveDefaultButton {
            id: saveButton

            onClicked: settings.audioOutCurve = curveCombo.currentValue
        }

        Item {
            Layout.fillWidth: true
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
