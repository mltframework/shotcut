/*
 * Copyright (c) 2019-2021 Meltytech, LLC
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
    property bool blockUpdate: true
    property double startWidthValue: 0
    property double middleWidthValue: 0.1
    property double endWidthValue: 0
    property double startHeightValue: 0
    property double middleHeightValue: 0.1
    property double endHeightValue: 0

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        wslider.value = filter.getDouble('0', position) * 100;
        hslider.value = filter.getDouble('1', position) * 100;
        widthKeyframesButton.checked = filter.keyframeCount('0') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        heightKeyframesButton.checked = filter.keyframeCount('1') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        blockUpdate = false;
        wslider.enabled = hslider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateFilterWidth(position) {
        if (blockUpdate)
            return ;

        var value = wslider.value / 100;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startWidthValue = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endWidthValue = value;
            else
                middleWidthValue = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('0');
            widthKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set('0', startWidthValue, 0);
                filter.set('0', middleWidthValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set('0', middleWidthValue, filter.duration - filter.animateOut);
                filter.set('0', endWidthValue, filter.duration - 1);
            }
        } else if (!widthKeyframesButton.checked) {
            filter.resetProperty('0');
            filter.set('0', middleWidthValue);
        } else if (position !== null) {
            filter.set('0', value, position);
        }
    }

    function updateFilterHeight(position) {
        if (blockUpdate)
            return ;

        var value = hslider.value / 100;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startHeightValue = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endHeightValue = value;
            else
                middleHeightValue = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('1');
            heightKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set('1', startHeightValue, 0);
                filter.set('1', middleHeightValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set('1', middleHeightValue, filter.duration - filter.animateOut);
                filter.set('1', endHeightValue, filter.duration - 1);
            }
        } else if (!heightKeyframesButton.checked) {
            filter.resetProperty('1');
            filter.set('1', middleHeightValue);
        } else if (position !== null) {
            filter.set('1', value, position);
        }
    }

    width: 200
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('0', 0.1);
            filter.set('1', 0.1);
            filter.savePreset(preset.parameters);
        } else {
            middleWidthValue = filter.getDouble('0', filter.animateIn);
            middleHeightValue = filter.getDouble('1', filter.animateIn);
            if (filter.animateIn > 0) {
                startWidthValue = filter.getDouble('0', 0);
                startHeightValue = filter.getDouble('1', 0);
            }
            if (filter.animateOut > 0) {
                endWidthValue = filter.getDouble('0', filter.duration - 1);
                endHeightValue = filter.getDouble('1', filter.duration - 1);
            }
        }
        setControls();
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

            Layout.columnSpan: parent.columns - 1
            parameters: ['0', '1']
            onBeforePresetLoaded: {
                filter.resetProperty('0');
                filter.resetProperty('1');
            }
            onPresetSelected: {
                setControls();
                middleWidthValue = filter.getDouble('0', filter.animateIn);
                middleHeightValue = filter.getDouble('1', filter.animateIn);
                if (filter.animateIn > 0) {
                    startWidthValue = filter.getDouble('0', 0);
                    startHeightValue = filter.getDouble('1', 0);
                }
                if (filter.animateOut > 0) {
                    endWidthValue = filter.getDouble('0', filter.duration - 1);
                    endHeightValue = filter.getDouble('1', filter.duration - 1);
                }
            }
        }

        Label {
            text: qsTr('Rows')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: wslider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilterWidth(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: wslider.value = 10
        }

        Shotcut.KeyframesButton {
            id: widthKeyframesButton

            onToggled: {
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty('1');
                        filter.set('1', middleHeightValue);
                        hslider.enabled = true;
                    }
                    filter.clearSimpleAnimation('0');
                    blockUpdate = false;
                    filter.set('0', wslider.value / 100, getPosition());
                } else {
                    filter.resetProperty('0');
                    filter.set('0', wslider.value / 100);
                }
            }
        }

        Label {
            text: qsTr('Columns')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: hslider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilterHeight(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hslider.value = 10
        }

        Shotcut.KeyframesButton {
            id: heightKeyframesButton

            onToggled: {
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty('0');
                        filter.set('0', middleWidthValue);
                        wslider.enabled = true;
                    }
                    filter.clearSimpleAnimation('1');
                    blockUpdate = false;
                    filter.set('1', hslider.value / 100, getPosition());
                } else {
                    filter.resetProperty('1');
                    filter.set('1', hslider.value / 100);
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

    }

    Connections {
        function onChanged() {
            setControls();
        }

        function onInChanged() {
            setControls();
            updateFilterWidth(null);
            updateFilterHeight(null);
        }

        function onOutChanged() {
            setControls();
            updateFilterWidth(null);
            updateFilterHeight(null);
        }

        function onAnimateInChanged() {
            setControls();
            updateFilterWidth(null);
            updateFilterHeight(null);
        }

        function onAnimateOutChanged() {
            setControls();
            updateFilterWidth(null);
            updateFilterHeight(null);
        }

        function onPropertyChanged() {
            setControls();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                setControls();
            } else {
                blockUpdate = true;
                wslider.value = filter.getDouble('0', getPosition()) * 100;
                hslider.value = filter.getDouble('1', getPosition()) * 100;
                blockUpdate = false;
                wslider.enabled = true;
                hslider.enabled = true;
            }
        }

        target: producer
    }

}
