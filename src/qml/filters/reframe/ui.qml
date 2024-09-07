/*
 * Copyright (c) 2024 Meltytech, LLC
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
import org.shotcut.qml as Shotcut

Shotcut.KeyframableFilter {
    property string startValueRect: '_shotcut:startValue'
    property string middleValueRect: '_shotcut:middleValue'
    property string endValueRect: '_shotcut:endValue'
    property string rectProperty: 'rect'
    property rect filterRect

    function setRectControls() {
        var position = getPosition();
        var newValue = filter.getRect(rectProperty, position);
        if (filterRect !== newValue) {
            filterRect = newValue;
            rectX.value = filterRect.x.toFixed();
            rectY.value = filterRect.y.toFixed();
            rectW.value = filterRect.width.toFixed();
            rectH.value = filterRect.height.toFixed();
        }
        var enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
        rectX.enabled = enabled;
        rectY.enabled = enabled;
        rectW.enabled = enabled;
        rectH.enabled = enabled;
        positionKeyframesButton.checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
    }

    function updateFilterRect(position) {
        var rect;
        if (position !== null) {
            filter.blockSignals = true;
            if (position <= 0 && filter.animateIn > 0)
                filter.set(startValueRect, filterRect);
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                filter.set(endValueRect, filterRect);
            else
                filter.set(middleValueRect, filterRect);
            filter.blockSignals = false;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(rectProperty);
            positionKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                rect = filter.getRect(startValueRect);
                filter.set(rectProperty, rect, 0);
                rect = filter.getRect(middleValueRect);
                filter.set(rectProperty, rect, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                rect = filter.getRect(middleValueRect);
                filter.set(rectProperty, rect, filter.duration - filter.animateOut);
                rect = filter.getRect(endValueRect);
                filter.set(rectProperty, rect, filter.duration - 1);
            }
        } else if (!positionKeyframesButton.checked) {
            filter.resetProperty(rectProperty);
            rect = filter.getRect(middleValueRect);
            filter.set(rectProperty, rect);
        } else if (position !== null) {
            filter.set(rectProperty, filterRect, position);
        }
    }

    function updateParameters() {
        updateFilterRect(null);
    }

    function applyTracking(motionTrackerRow, operation, frame) {
        motionTrackerModel.reset(filter, rectProperty, motionTrackerRow);
        const data = motionTrackerModel.trackingData(motionTrackerRow);
        let previous = null;
        let interval = motionTrackerModel.keyframeIntervalFrames(motionTrackerRow);
        let interpolation = Shotcut.KeyframesModel.SmoothNaturalInterpolation;
        filter.blockSignals = true;
        data.forEach(i => {
            let current = filter.getRect(rectProperty, frame);
            let x = 0;
            let y = 0;
            if (previous !== null) {
                x = i.x - previous.x;
                y = i.y - previous.y;
            }
            switch (operation) {
            case 'relativePos':
                current.x += x;
                current.y += y;
                break;
            case 'offsetPos':
                current.x -= x;
                current.y -= y;
                break;
            case 'absPos':
                current.x = i.x + i.width / 2 - current.width / 2;
                current.y = i.y + i.height / 2 - current.height / 2;
                interpolation = Shotcut.KeyframesModel.LinearInterpolation;
                break;
            case 'absSizePos':
                current.x = i.x;
                current.y = i.y;
                if (previous === null) {
                    current.width = i.width;
                    current.height = i.height;
                }
                interpolation = Shotcut.KeyframesModel.LinearInterpolation;
                break;
            }
            previous = i;
            current.x = Math.min(Math.max(current.x, 0), profile.width - current.width);
            current.y = Math.min(Math.max(current.y, 0), profile.height - current.height);
            filter.set(rectProperty, current, frame, interpolation);
            frame += interval;
        });
        filter.blockSignals = false;
        parameters.reload();
    }

    width: 400
    height: 100
    Component.onCompleted: {
        filter.blockSignals = true;
        let width = profile.height * 9/16;
        width += width % 2;
        let rect = Qt.rect(0.5 * (profile.width - width), 0, width, profile.height);
        filter.set(startValueRect, rect);
        filter.set(middleValueRect, rect);
        filter.set(endValueRect, rect);
        filter.set('filter', '0');
        if (filter.isNew) {
            width = profile.height * 4/3;
            width += width % 2;
            rect = Qt.rect(0.5 * (profile.width - width), 0, width, profile.height);
            filter.set(rectProperty, rect);
            filter.savePreset(preset.parameters, qsTr('Horizontal 4:3'));

            width = profile.height * 16/9;
            width += width % 2;
            rect = Qt.rect(0.5 * (profile.width - width), 0, width, profile.height);
            filter.set(rectProperty, rect);
            filter.savePreset(preset.parameters, qsTr('Horizontal 16:9'));

            width = profile.height;
            width += width % 2;
            rect = Qt.rect(0.5 * (profile.width - width), 0, width, profile.height);
            filter.set(rectProperty, rect);
            filter.savePreset(preset.parameters, qsTr('Square'));

            // Set default parameter values
            width = profile.height * 9/16;
            width += width % 2;
            rect = Qt.rect(0.5 * (profile.width - width), 0, width, profile.height);
            filter.set(rectProperty, rect);
            filter.savePreset(preset.parameters, qsTr('Vertical 9:16'));
        } else {
            filter.set(middleValueRect, filter.getRect(rectProperty, filter.animateIn + 1));
            if (filter.animateIn > 0) {
                filter.set(startValueRect, filter.getRect(rectProperty, 0));
            }
            if (filter.animateOut > 0) {
                filter.set(endValueRect, filter.getRect(rectProperty, filter.duration - 1));
            }
        }
        filter.blockSignals = false;
        setRectControls();
        if (filter.isNew)
            filter.set(rectProperty, filter.getRect(rectProperty));
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: [rectProperty]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filterRect = Qt.rect(0, 0, 0, 0);
                filter.resetProperty(rectProperty);
                resetSimpleKeyframes();
            }
            onPresetSelected: {
                setRectControls();
                initializeSimpleKeyframes();
                filter.blockSignals = true;
                filter.set(middleValueRect, filter.getRect(rectProperty, filter.animateIn + 1));
                if (filter.animateIn > 0) {
                    filter.set(startValueRect, filter.getRect(rectProperty, 0));
                }
                if (filter.animateOut > 0) {
                    filter.set(endValueRect, filter.getRect(rectProperty, filter.duration - 1));
                }
                filter.blockSignals = false;
            }
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Shotcut.DoubleSpinBox {
                id: rectX

                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: 0
                to: profile.width - filterRect.width
                onValueModified: {
                    if (filterRect.x !== value) {
                        filterRect.x = value;
                        updateFilterRect(getPosition(), true);
                    }
                }
            }

            Label {
                text: ','
                Layout.minimumWidth: 20
                horizontalAlignment: Qt.AlignHCenter
            }

            Shotcut.DoubleSpinBox {
                id: rectY

                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: 0
                to: profile.height - filterRect.height
                onValueModified: {
                    if (filterRect.y !== value) {
                        filterRect.y = value;
                        updateFilterRect(getPosition(), true);
                    }
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                let width = profile.height * 9 / 16;
                width += width % 2;
                rectX.value = 0.5 * (profile.width - width);
                rectY.value = 0;
                filterRect.x = rectX.value;
                filterRect.y = 0;
                updateFilterRect(getPosition(), true);
            }
        }

        Shotcut.KeyframesButton {
            id: positionKeyframesButton

            Layout.rowSpan: 2
            onToggled: {
                if (checked) {
                    filter.blockSignals = true;
                    filter.clearSimpleAnimation(rectProperty);
                    filter.blockSignals = false;
                    filter.set(rectProperty, filterRect, getPosition());
                } else {
                    filter.blockSignals = true;
                    filter.resetProperty(rectProperty);
                    filter.blockSignals = false;
                    filter.set(rectProperty, filterRect);
                }
                checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
            }
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Shotcut.DoubleSpinBox {
                id: rectW

                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: 2
                to: profile.width - filterRect.x
                onValueModified: {
                    if (filterRect.width !== value) {
                        value += value % 2 * (value > filterRect.width ? 1 : -1)
                        filterRect.width = value;
                        updateFilterRect(getPosition(), true);
                    }
                }
            }

            Label {
                text: 'x'
                Layout.minimumWidth: 20
                horizontalAlignment: Qt.AlignHCenter
            }

            Shotcut.DoubleSpinBox {
                id: rectH

                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: 2
                to: profile.height - filterRect.y
                onValueModified: {
                    if (filterRect.height !== value) {
                        value += value % 2 * (value > filterRect.height ? 1 : -1)
                        filterRect.height = value;
                        updateFilterRect(getPosition(), true);
                    }
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                let width = profile.height * 9 / 16;
                width += width % 2;
                rectW.value = width;
                rectH.value = profile.height;
                filterRect.width = rectW.value;
                filterRect.height = profile.height;
                updateFilterRect(getPosition(), true);
            }
        }

        Item {
            width: 1
        }

        Shotcut.Button {
            Layout.columnSpan: parent.columns - 1
            text: motionTrackerDialog.title
            onClicked: motionTrackerDialog.show()
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Shotcut.MotionTrackerDialog {
        id: motionTrackerDialog
        onAccepted: (motionTrackerRow, operation, startFrame) => applyTracking(motionTrackerRow, operation, startFrame)
        onReset: if (filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0) {
            filter.startUndoParameterCommand(title);
            motionTrackerModel.undo(filter, rectProperty);
            filterRect = filter.getRect(rectProperty, getPosition());
            rectX.value = filterRect.x;
            rectY.value = filterRect.y;
            rectW.value = filterRect.width;
            rectH.value = filterRect.height;
            updateFilterRect(getPosition());
            filter.endUndoCommand();
        }
    }

    Connections {
        function onChanged() {
            setRectControls();
        }

        function onInChanged() {
            updateParameters();
        }

        function onOutChanged() {
            updateParameters();
        }

        function onAnimateInChanged() {
            updateParameters();
        }

        function onAnimateOutChanged() {
            updateParameters();
        }

        function onPropertyChanged(name) {
            setRectControls();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            setRectControls();
        }

        target: producer
    }
}
