/*
 * Copyright (c) 2018 Meltytech, LLC
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

import QtQuick 2.0
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0
import QtQml.Models 2.2

Item {
    width: 500
    height: 350

    property string _defaultStart: '00:00:00.000'
    property string _defaultDuration: '00:00:10.000'

    Component.onCompleted: {
        filter.blockSignals = true
        filter.set(textFilterUi.middleValue, Qt.rect(0, 0, profile.width, profile.height))
        filter.set(textFilterUi.startValue, Qt.rect(0, 0, profile.width, profile.height))
        filter.set(textFilterUi.endValue, Qt.rect(0, 0, profile.width, profile.height))
        if (filter.isNew) {
            filter.set("start", _defaultStart)
            filter.set("duration", _defaultDuration)

            if (application.OS === 'Windows')
                filter.set('family', 'Verdana')
            filter.set('fgcolour', '#ffffffff')
            filter.set('bgcolour', '#00000000')
            filter.set('olcolour', '#ff000000')
            filter.set('weight', 10 * Font.Normal)
            filter.set('style', 'normal')
            filter.set(textFilterUi.useFontSizeProperty, false)
            filter.set('size', profile.height)

            filter.set(textFilterUi.rectProperty,   '0%/75%:25%x25%')
            filter.set(textFilterUi.valignProperty, 'bottom')
            filter.set(textFilterUi.halignProperty, 'left')
            filter.savePreset(preset.parameters, qsTr('Bottom Left'))

            filter.set(textFilterUi.rectProperty,   '75%/75%:25%x25%')
            filter.set(textFilterUi.valignProperty, 'bottom')
            filter.set(textFilterUi.halignProperty, 'right')
            filter.savePreset(preset.parameters, qsTr('Bottom Right'))

            // Add default preset.
            filter.set(textFilterUi.rectProperty, '0%/0%:100%x100%')
            filter.savePreset(preset.parameters)
        } else {
            filter.set(textFilterUi.middleValue, filter.getRect(textFilterUi.rectProperty, filter.animateIn + 1))
            if (filter.animateIn > 0)
                filter.set(textFilterUi.startValue, filter.getRect(textFilterUi.rectProperty, 0))
            if (filter.animateOut > 0)
                filter.set(textFilterUi.endValue, filter.getRect(textFilterUi.rectProperty, filter.duration - 1))
        }
        filter.blockSignals = false
        setControls()
        if (filter.isNew)
            filter.set(textFilterUi.rectProperty, filter.getRect(textFilterUi.rectProperty))
    }

    function setControls() {
        var formatIndex = 0;
        var format = filter.get('format')
        for (var i = 0; i < formatCombo.model.count; i++) {
            if (formatCombo.model.get(i).format == format) {
                formatIndex = i
                break
            }
        }
        formatCombo.currentIndex = formatIndex

        var directionIndex = 0;
        var direction = filter.get('direction')
        for (var i = 0; i < directionCombo.model.count; i++) {
            if (directionCombo.model.get(i).direction == direction) {
                directionIndex = i
                break
            }
        }
        directionCombo.currentIndex = directionIndex

        startSpinner.timeStr = filter.get("start")
        durationSpinner.timeStr = filter.get("duration")

        textFilterUi.setControls()
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
        Preset {
            id: preset
            parameters: textFilterUi.parameters.concat(['format', 'direction','start','duration'])
            onBeforePresetLoaded: {
                filter.resetProperty(textFilterUi.rectProperty)
            }
            onPresetSelected: {
                setControls()
                filter.blockSignals = true
                filter.set(textFilterUi.middleValue, filter.getRect(textFilterUi.rectProperty, filter.animateIn + 1))
                if (filter.animateIn > 0)
                    filter.set(textFilterUi.startValue, filter.getRect(textFilterUi.rectProperty, 0))
                if (filter.animateOut > 0)
                    filter.set(textFilterUi.endValue, filter.getRect(textFilterUi.rectProperty, filter.duration - 1))
                filter.blockSignals = false
            }
        }

        Label {
            text: qsTr('Format')
            Layout.alignment: Qt.AlignRight
        }
        ComboBox {
            id: formatCombo
            model: ListModel {
                ListElement { text: QT_TR_NOOP('HH:MM:SS'); format: "HH:MM:SS" }
                ListElement { text: QT_TR_NOOP('HH:MM:SS.S'); format: "HH:MM:SS.S" }
                ListElement { text: QT_TR_NOOP('MM:SS'); format: "MM:SS" }
                ListElement { text: QT_TR_NOOP('MM:SS.SS'); format: "MM:SS.SS" }
                ListElement { text: QT_TR_NOOP('SS'); format: "SS" }
                ListElement { text: QT_TR_NOOP('SS.S'); format: "SS.S" }
                ListElement { text: QT_TR_NOOP('SS.SS'); format: "SS.SS" }
            }
            onCurrentIndexChanged: {
                filter.set('format', model.get(currentIndex).format)
            }
        }

        Label {
            text: qsTr('Direction')
            Layout.alignment: Qt.AlignRight
        }
        ComboBox {
            id: directionCombo
            model: ListModel {
                ListElement { text: QT_TR_NOOP('Up'); direction: "up" }
                ListElement { text: QT_TR_NOOP('Down'); direction: "down" }
            }
            onCurrentIndexChanged: {
                filter.set('direction', model.get(currentIndex).direction)
            }
        }

        Label {
            text: qsTr('Start')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            spacing: 0
            ClockSpinner {
                id: startSpinner
                maximumValue: 10 * 60 * 60 // 10 hours
                onTimeStrChanged: {
                    filter.set('start', startSpinner.timeStr)
                }
                onSetDefaultClicked: {
                    startSpinner.timeStr = _defaultStart
                }
            }
            Button {
                iconName: 'insert'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/insert.png'
                tooltip: qsTr('Set start to current position')
                implicitWidth: 20
                implicitHeight: 20
                onClicked: startSpinner.setValueSeconds(producer.position / profile.fps)
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
                maximumValue: 10 * 60 * 60 // 10 hours
                onTimeStrChanged: {
                    filter.set('duration', durationSpinner.timeStr)
                }
                onSetDefaultClicked: {
                    durationSpinner.timeStr = _defaultDuration
                }
            }
            Button {
                iconName: 'insert'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/insert.png'
                tooltip: qsTr('Set end to current position')
                implicitWidth: 20
                implicitHeight: 20
                onClicked: {
                    var startTime = startSpinner.getValueSeconds()
                    var endTime = producer.position / profile.fps
                    if (endTime > startTime) {
                        durationSpinner.setValueSeconds(endTime - startTime)
                    }
                }
            }
        }

        TextFilterUi {
            id: textFilterUi
            Layout.columnSpan: 2
        }

        Item { Layout.fillHeight: true }
    }
}
