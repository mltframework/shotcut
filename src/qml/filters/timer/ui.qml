/*
 * Copyright (c) 2018-2022 Meltytech, LLC
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

import QtQml.Models 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    property string _defaultStart: '00:00:00.000'
    property string _defaultDuration: '00:00:10.000'
    property string _defaultOffset: '00:00:00.000'
    property double _defaultSpeed: 1

    function setControls() {
        var formatIndex = 0;
        var format = filter.get('format');
        for (var i = 0; i < formatCombo.model.count; i++) {
            if (formatCombo.model.get(i).format === format) {
                formatIndex = i;
                break;
            }
        }
        formatCombo.currentIndex = formatIndex;
        var directionIndex = 0;
        var direction = filter.get('direction');
        for (var i = 0; i < directionCombo.model.count; i++) {
            if (directionCombo.model.get(i).direction === direction) {
                directionIndex = i;
                break;
            }
        }
        directionCombo.currentIndex = directionIndex;
        startSpinner.timeStr = filter.get("start");
        durationSpinner.timeStr = filter.get("duration");
        offsetSpinner.timeStr = filter.get("offset");
        speedSpinner.value = filter.getDouble("speed");
        textFilterUi.setControls();
    }

    width: 400
    height: 425
    Component.onCompleted: {
        filter.blockSignals = true;
        filter.set(textFilterUi.middleValue, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(textFilterUi.startValue, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(textFilterUi.endValue, Qt.rect(0, 0, profile.width, profile.height));
        if (filter.isNew) {
            filter.set("start", _defaultStart);
            filter.set("duration", _defaultDuration);
            filter.set("offset", _defaultOffset);
            filter.set("speed", _defaultSpeed);
            if (application.OS === 'Windows')
                filter.set('family', 'Verdana');

            filter.set('fgcolour', '#ffffffff');
            filter.set('bgcolour', '#00000000');
            filter.set('olcolour', '#ff000000');
            filter.set('weight', 10 * Font.Normal);
            filter.set('style', 'normal');
            filter.set(textFilterUi.useFontSizeProperty, false);
            filter.set('size', profile.height);
            filter.set(textFilterUi.rectProperty, '0%/75%:25%x25%');
            filter.set(textFilterUi.valignProperty, 'bottom');
            filter.set(textFilterUi.halignProperty, 'left');
            filter.savePreset(preset.parameters, qsTr('Bottom Left'));
            filter.set(textFilterUi.rectProperty, '75%/75%:25%x25%');
            filter.set(textFilterUi.valignProperty, 'bottom');
            filter.set(textFilterUi.halignProperty, 'right');
            filter.savePreset(preset.parameters, qsTr('Bottom Right'));
            // Add default preset.
            filter.set(textFilterUi.rectProperty, '0%/0%:100%x100%');
            filter.savePreset(preset.parameters);
        } else {
            filter.set(textFilterUi.middleValue, filter.getRect(textFilterUi.rectProperty, filter.animateIn + 1));
            if (filter.animateIn > 0)
                filter.set(textFilterUi.startValue, filter.getRect(textFilterUi.rectProperty, 0));

            if (filter.animateOut > 0)
                filter.set(textFilterUi.endValue, filter.getRect(textFilterUi.rectProperty, filter.duration - 1));

        }
        filter.blockSignals = false;
        setControls();
        if (filter.isNew)
            filter.set(textFilterUi.rectProperty, filter.getRect(textFilterUi.rectProperty));

    }

    GridLayout {
        id: textGrid

        columns: 2
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: textFilterUi.parameterList.concat(['format', 'direction', 'start', 'duration'])
            onBeforePresetLoaded: {
                filter.resetProperty(textFilterUi.rectProperty);
            }
            onPresetSelected: {
                setControls();
                filter.blockSignals = true;
                filter.set(textFilterUi.middleValue, filter.getRect(textFilterUi.rectProperty, filter.animateIn + 1));
                if (filter.animateIn > 0)
                    filter.set(textFilterUi.startValue, filter.getRect(textFilterUi.rectProperty, 0));

                if (filter.animateOut > 0)
                    filter.set(textFilterUi.endValue, filter.getRect(textFilterUi.rectProperty, filter.duration - 1));

                filter.blockSignals = false;
            }
        }

        Label {
            text: qsTr('Format')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: formatCombo

            textRole: 'text'
            onActivated: {
                filter.set('format', model.get(currentIndex).format);
            }

            model: ListModel {
                ListElement {
                    text: QT_TR_NOOP('HH:MM:SS')
                    format: "HH:MM:SS"
                }

                ListElement {
                    text: QT_TR_NOOP('HH:MM:SS.S')
                    format: "HH:MM:SS.S"
                }

                ListElement {
                    text: QT_TR_NOOP('MM:SS')
                    format: "MM:SS"
                }

                ListElement {
                    text: QT_TR_NOOP('MM:SS.SS')
                    format: "MM:SS.SS"
                }

                ListElement {
                    text: QT_TR_NOOP('MM:SS.SSS')
                    format: "MM:SS.SSS"
                }

                ListElement {
                    text: QT_TR_NOOP('SS')
                    format: "SS"
                }

                ListElement {
                    text: QT_TR_NOOP('SS.S')
                    format: "SS.S"
                }

                ListElement {
                    text: QT_TR_NOOP('SS.SS')
                    format: "SS.SS"
                }

                ListElement {
                    text: QT_TR_NOOP('SS.SSS')
                    format: "SS.SSS"
                }

            }

        }

        Label {
            text: qsTr('Direction')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: directionCombo

            textRole: 'text'
            onActivated: {
                filter.set('direction', model.get(currentIndex).direction);
            }

            model: ListModel {
                ListElement {
                    text: QT_TR_NOOP('Up')
                    direction: "up"
                }

                ListElement {
                    text: QT_TR_NOOP('Down')
                    direction: "down"
                }

            }

        }

        Label {
            text: qsTr('Start Delay')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            spacing: 0

            ClockSpinner {
                id: startSpinner

                maximumValue: 100 * 60 * 60 - 0.001 // 99:59:59.999
                onTimeStrChanged: {
                    filter.set('start', startSpinner.timeStr);
                }
                onSetDefaultClicked: {
                    startSpinner.timeStr = _defaultStart;
                }

                Shotcut.HoverTip {
                    text: qsTr('The timer will be frozen from the beginning of the filter until the Start Delay time has elapsed.')
                }

            }

            Shotcut.Button {
                icon.name: 'insert'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/insert.png'
                implicitWidth: 20
                implicitHeight: 20
                onClicked: startSpinner.setValueSeconds((producer.position - (filter.in - producer.in)) / profile.fps)

                Shotcut.HoverTip {
                    text: qsTr('Set start to begin at the current position')
                }

            }

        }

        Label {
            text: qsTr('Duration')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            spacing: 0

            ClockSpinner {
                id: durationSpinner

                maximumValue: 100 * 60 * 60 - 0.001 // 99:59:59.999
                onTimeStrChanged: {
                    filter.set('duration', durationSpinner.timeStr);
                }
                onSetDefaultClicked: {
                    durationSpinner.timeStr = _defaultDuration;
                }

                Shotcut.HoverTip {
                    text: qsTr('The timer will be frozen after the Duration has elapsed.') + '\n' + qsTr('A value of 0 will run the timer to the end of the filter')
                }

            }

            Shotcut.Button {
                icon.name: 'insert'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/insert.png'
                implicitWidth: 20
                implicitHeight: 20
                onClicked: {
                    var startTime = startSpinner.getValueSeconds();
                    var endTime = (producer.position - (filter.in - producer.in)) / profile.fps;
                    if (endTime > startTime)
                        durationSpinner.setValueSeconds(endTime - startTime);

                }

                Shotcut.HoverTip {
                    text: qsTr('Set duration to end at the current position')
                }

            }

        }

        Label {
            text: qsTr('Offset')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            spacing: 0

            ClockSpinner {
                id: offsetSpinner

                maximumValue: 100 * 60 * 60 - 0.001 // 99:59:59.999
                onTimeStrChanged: {
                    filter.set('offset', offsetSpinner.timeStr);
                }
                onSetDefaultClicked: {
                    offsetSpinner.timeStr = _defaultOffset;
                }

                Shotcut.HoverTip {
                    text: qsTr('When the direction is Down, the timer will count down to Offset.\nWhen the direction is Up, the timer will count up starting from Offset.')
                }

            }

        }

        Label {
            text: qsTr('Speed')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            spacing: 0

            Shotcut.DoubleSpinBox {
                id: speedSpinner

                horizontalAlignment: Qt.AlignRight
                Layout.minimumWidth: 100
                decimals: 5
                stepSize: 0.1
                from: 0
                to: 1000
                onValueChanged: {
                    filter.set("speed", speedSpinner.value);
                }

                Shotcut.HoverTip {
                    text: qsTr('Timer seconds per playback second. Scales Duration but does not affect Start Delay or Offset.')
                }

            }

        }

        Shotcut.TextFilterUi {
            id: textFilterUi

            Layout.columnSpan: 2
        }

        Item {
            Layout.fillHeight: true
        }

    }

}
