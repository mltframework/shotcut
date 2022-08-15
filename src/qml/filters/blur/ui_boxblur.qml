/*
 * Copyright (c) 2013-2021 Meltytech, LLC
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
    property int startWidthValue: 1
    property int middleWidthValue: 2
    property int endWidthValue: 1
    property int startHeightValue: 1
    property int middleHeightValue: 2
    property int endHeightValue: 1

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        wslider.value = filter.getDouble('hori', position);
        widthKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('hori') > 0;
        hslider.value = filter.getDouble('vert', position);
        heightKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('vert') > 0;
        blockUpdate = false;
        wslider.enabled = hslider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateFilterWidth(position) {
        if (blockUpdate)
            return ;

        var value = wslider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startWidthValue = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endWidthValue = value;
            else
                middleWidthValue = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('hori');
            widthKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set('hori', startWidthValue, 0);
                filter.set('hori', middleWidthValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set('hori', middleWidthValue, filter.duration - filter.animateOut);
                filter.set('hori', endWidthValue, filter.duration - 1);
            }
        } else if (!widthKeyframesButton.checked) {
            filter.resetProperty('hori');
            filter.set('hori', middleWidthValue);
        } else if (position !== null) {
            filter.set('hori', value, position);
        }
    }

    function updateFilterHeight(position) {
        if (blockUpdate)
            return ;

        var value = hslider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startHeightValue = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endHeightValue = value;
            else
                middleHeightValue = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('vert');
            heightKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set('vert', startHeightValue, 0);
                filter.set('vert', middleHeightValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set('vert', middleHeightValue, filter.duration - filter.animateOut);
                filter.set('vert', endHeightValue, filter.duration - 1);
            }
        } else if (!heightKeyframesButton.checked) {
            filter.resetProperty('vert');
            filter.set('vert', middleHeightValue);
        } else if (position !== null) {
            filter.set('vert', value, position);
        }
    }

    width: 200
    height: 50
    Component.onCompleted: {
        filter.set('start', 1);
        if (filter.isNew) {
            // Set default parameter values
            filter.set('hori', 2);
            filter.set('vert', 2);
            filter.savePreset(preset.parameters);
        } else {
            middleWidthValue = filter.getDouble('hori', filter.animateIn);
            middleHeightValue = filter.getDouble('vert', filter.animateIn);
            if (filter.animateIn > 0) {
                startWidthValue = filter.getDouble('hori', 0);
                startHeightValue = filter.getDouble('vert', 0);
            }
            if (filter.animateOut > 0) {
                endWidthValue = filter.getDouble('hori', filter.duration - 1);
                endHeightValue = filter.getDouble('vert', filter.duration - 1);
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
            parameters: ['hori', 'vert']
            onBeforePresetLoaded: {
                filter.resetProperty('hori');
                filter.resetProperty('vert');
            }
            onPresetSelected: {
                setControls();
                widthKeyframesButton.checked = filter.keyframeCount('hori') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
                heightKeyframesButton.checked = filter.keyframeCount('vert') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
                middleWidthValue = filter.getDouble('hori', filter.animateIn);
                middleHeightValue = filter.getDouble('vert', filter.animateIn);
                if (filter.animateIn > 0) {
                    startWidthValue = filter.getDouble('hori', 0);
                    startHeightValue = filter.getDouble('vert', 0);
                }
                if (filter.animateOut > 0) {
                    endWidthValue = filter.getDouble('hori', filter.duration - 1);
                    endHeightValue = filter.getDouble('vert', filter.duration - 1);
                }
            }
        }

        Label {
            text: qsTr('Width')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: wslider

            minimumValue: 0
            maximumValue: 99
            suffix: ' px'
            onValueChanged: updateFilterWidth(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: wslider.value = 2
        }

        Shotcut.KeyframesButton {
            id: widthKeyframesButton

            onToggled: {
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty('vert');
                        filter.set('vert', middleHeightValue);
                        hslider.enabled = true;
                    }
                    filter.clearSimpleAnimation('hori');
                    blockUpdate = false;
                    filter.set('hori', wslider.value, getPosition());
                } else {
                    filter.resetProperty('hori');
                    filter.set('hori', wslider.value);
                }
            }
        }

        Label {
            text: qsTr('Height')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: hslider

            minimumValue: 0
            maximumValue: 99
            suffix: ' px'
            onValueChanged: updateFilterHeight(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hslider.value = 2
        }

        Shotcut.KeyframesButton {
            id: heightKeyframesButton

            onToggled: {
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty('hori');
                        filter.set('hori', middleWidthValue);
                        wslider.enabled = true;
                    }
                    filter.clearSimpleAnimation('vert');
                    blockUpdate = false;
                    filter.set('vert', hslider.value, getPosition());
                } else {
                    filter.resetProperty('vert');
                    filter.set('vert', hslider.value);
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
            updateSimpleKeyframes();
        }

        function onOutChanged() {
            updateSimpleKeyframes();
        }

        function onAnimateInChanged() {
            updateSimpleKeyframes();
        }

        function onAnimateOutChanged() {
            updateSimpleKeyframes();
        }

        function onPropertyChanged(name) {
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
                wslider.value = filter.getDouble('hori', getPosition());
                hslider.value = filter.getDouble('vert', getPosition());
                blockUpdate = false;
                wslider.enabled = true;
                hslider.enabled = true;
            }
        }

        target: producer
    }

}
