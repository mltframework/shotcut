/*
 * Copyright (c) 2018-2026 Meltytech, LLC
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
import QtQuick.Dialogs
import QtQuick.Layouts
import Shotcut.Controls as Shotcut
import org.shotcut.qml as Shotcut

Item {
    id: shapeRoot

    property bool blockUpdate: true
    property double startValue: 0
    property double middleValue: 100
    property double endValue: 0
    property int previousResourceComboIndex
    property string reverseProperty: filter.isAtLeastVersion(3) ? 'filter.invert_mask' : 'filter.reverse'

    function initSimpleAnimation() {
        middleValue = filter.getDouble('filter.mix', filter.animateIn);
        if (filter.animateIn > 0)
            startValue = filter.getDouble('filter.mix', 0);
        if (filter.animateOut > 0)
            endValue = filter.getDouble('filter.mix', filter.duration - 1);
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setKeyframedControls() {
        var position = getPosition();
        blockUpdate = true;
        thresholdSlider.value = filter.getDouble('filter.mix', position);
        thresholdKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('filter.mix') > 0;
        blockUpdate = false;
        thresholdSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function setControls() {
        setKeyframedControls();
        shapeFile.url = filter.get('filter.resource');
        if (shapeFile.fileName.substring(0, 5) === '%luma') {
            for (var i = 1; i < resourceCombo.model.count; ++i) {
                var s = (i < 10) ? '%luma0%1.pgm' : '%luma%1.pgm';
                if (s.arg(i) === shapeFile.fileName) {
                    resourceCombo.currentIndex = (i === 1) ? 0 : i;
                    break;
                }
            }
            alphaRadioButton.enabled = false;
        } else {
            resourceCombo.currentIndex = -1;
            for (var i = 0; i < application.wipes.length; ++i) {
                if (shapeFile.filePath === application.wipes[i]) {
                    resourceCombo.currentIndex = i + 23;
                    console.log('resource.currentIndex = ' + i);
                    break;
                }
            }
            if (resourceCombo.currentIndex === -1) {
                resourceCombo.currentIndex = 1;
                fileLabel.text = shapeFile.fileName;
                fileLabelTip.text = shapeFile.filePath;
            }
            alphaRadioButton.enabled = true;
        }
        previousResourceComboIndex = resourceCombo.currentIndex;
        thresholdCheckBox.checked = filter.getDouble('filter.use_mix') === 1;
        invertCheckBox.checked = filter.getDouble('filter.invert') === 1;
        reverseCheckBox.checked = filter.getDouble(reverseProperty) === 1;
        if (filter.getDouble('filter.use_luminance') === 1)
            brightnessRadioButton.checked = true;
        else
            alphaRadioButton.checked = true;
        softnessSlider.value = filter.getDouble('filter.softness') * 100;
        var currentOp = filter.get('filter.alpha_operation');
        for (var i = 0; i < operationModel.count; ++i) {
            if (operationModel.get(i).value === currentOp) {
                operationCombo.currentIndex = i;
                break;
            }
        }
    }

    function updateFilter(parameter, value, position, button) {
        if (blockUpdate)
            return;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = value;
            else
                middleValue = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(parameter);
            button.checked = false;
            if (filter.animateIn > 0) {
                filter.set(parameter, startValue, 0);
                filter.set(parameter, middleValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set(parameter, middleValue, filter.duration - filter.animateOut);
                filter.set(parameter, endValue, filter.duration - 1);
            }
        } else if (!button.checked) {
            filter.resetProperty(parameter);
            filter.set(parameter, middleValue);
        } else if (position !== null) {
            filter.set(parameter, value, position);
        }
    }

    function onKeyframesButtonClicked(checked, parameter, value) {
        if (checked) {
            blockUpdate = true;
            thresholdSlider.enabled = softnessSlider.enabled = true;
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                filter.resetProperty('filter.mix');
                filter.animateIn = filter.animateOut = 0;
            } else {
                filter.clearSimpleAnimation(parameter);
            }
            blockUpdate = false;
            filter.set(parameter, value, getPosition());
        } else {
            filter.resetProperty(parameter);
            filter.set(parameter, value);
        }
    }

    function updatedSimpleAnimation() {
        setControls();
        updateFilter('filter.mix', thresholdSlider.value, null, thresholdKeyframesButton);
    }

    width: 350
    height: 275
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('filter', 'shape');
            filter.set('filter.mix', 50);
            filter.set('filter.softness', 0);
            filter.set('filter.invert', 0);
            filter.set('filter.use_luminance', 1);
            filter.set('filter.resource', '%luma01.pgm');
            filter.set('filter.use_mix', 1);
            filter.set('filter.audio_match', 0);
            filter.set('filter.alpha_operation', 'overwrite');
            filter.savePreset(preset.parameters);
        } else {
            if (filter.get('filter.use_mix').length === 0)
                filter.set('filter.use_mix', 1);
            if (filter.get('filter.audio_match').length === 0)
                filter.set('filter.audio_match', 1);
            initSimpleAnimation();
        }
        setControls();
    }

    Shotcut.File {
        id: shapeFile
    }

    Shotcut.FileDialog {
        id: fileDialog

        onAccepted: {
            shapeFile.url = fileDialog.selectedFile;
            filter.set('filter.resource', shapeFile.url);
            fileLabel.text = shapeFile.fileName;
            fileLabelTip.text = shapeFile.filePath;
            previousResourceComboIndex = resourceCombo.currentIndex;
            alphaRadioButton.enabled = true;
            settings.openPath = shapeFile.path;
        }
        onRejected: resourceCombo.currentIndex = previousResourceComboIndex
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

            Layout.columnSpan: 3
            parameters: ['filter.mix', 'filter.softness', 'filter.use_luminance', 'filter.invert', 'filter.resource', 'filter.use_mix', reverseProperty, 'filter.alpha_operation']
            onBeforePresetLoaded: {
                filter.resetProperty('filter.mix');
            }
            onPresetSelected: {
                setControls();
                initSimpleAnimation();
            }
        }

        Label {
            text: qsTr('File')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.File {
            id: wipeFile
        }

        Shotcut.ComboBox {
            id: resourceCombo

            function updateResource(index) {
                fileLabel.text = '';
                fileLabelTip.text = '';
                if (index === 1) {
                    fileDialog.title = qsTr('Open Mask File');
                    fileDialog.open();
                } else {
                    var s = resourceCombo.model.get(index).value;
                    filter.set('filter.resource', s);
                    previousResourceComboIndex = index;
                    brightnessRadioButton.checked = true;
                    filter.set('filter.use_luminance', 1);
                    alphaRadioButton.enabled = index >= 23;
                }
            }

            implicitWidth: 300
            textRole: 'text'
            currentIndex: 0
            onActivated: {
                // toggling focus works around a weird bug involving sticky
                // input event focus on the ComboBox
                enabled = false;
                updateResource(currentIndex);
                enabled = true;
            }

            Shotcut.HoverTip {
                text: qsTr('Set a mask from another file\'s brightness or alpha.')
                visible: !resourceCombo.pressed
            }

            model: ListModel {
                id: listModel

                Component.onCompleted: {
                    application.wipes.forEach(function (el) {
                        wipeFile.url = el;
                        append({
                            "text": wipeFile.fileName,
                            "value": el
                        });
                    });
                }

                ListElement {
                    text: qsTr('Bar Horizontal')
                    value: '%luma01.pgm'
                }

                ListElement {
                    text: qsTr('Custom...')
                    value: ''
                }

                ListElement {
                    text: qsTr('Bar Vertical')
                    value: '%luma02.pgm'
                }

                ListElement {
                    text: qsTr('Barn Door Horizontal')
                    value: '%luma03.pgm'
                }

                ListElement {
                    text: qsTr('Barn Door Vertical')
                    value: '%luma04.pgm'
                }

                ListElement {
                    text: qsTr('Barn Door Diagonal SW-NE')
                    value: '%luma05.pgm'
                }

                ListElement {
                    text: qsTr('Barn Door Diagonal NW-SE')
                    value: '%luma06.pgm'
                }

                ListElement {
                    text: qsTr('Diagonal Top Left')
                    value: '%luma07.pgm'
                }

                ListElement {
                    text: qsTr('Diagonal Top Right')
                    value: '%luma08.pgm'
                }

                ListElement {
                    text: qsTr('Matrix Waterfall Horizontal')
                    value: '%luma09.pgm'
                }

                ListElement {
                    text: qsTr('Matrix Waterfall Vertical')
                    value: '%luma10.pgm'
                }

                ListElement {
                    text: qsTr('Matrix Snake Horizontal')
                    value: '%luma11.pgm'
                }

                ListElement {
                    text: qsTr('Matrix Snake Parallel Horizontal')
                    value: '%luma12.pgm'
                }

                ListElement {
                    text: qsTr('Matrix Snake Vertical')
                    value: '%luma13.pgm'
                }

                ListElement {
                    text: qsTr('Matrix Snake Parallel Vertical')
                    value: '%luma14.pgm'
                }

                ListElement {
                    text: qsTr('Barn V Up')
                    value: '%luma15.pgm'
                }

                ListElement {
                    text: qsTr('Iris Circle')
                    value: '%luma16.pgm'
                }

                ListElement {
                    text: qsTr('Double Iris')
                    value: '%luma17.pgm'
                }

                ListElement {
                    text: qsTr('Iris Box')
                    value: '%luma18.pgm'
                }

                ListElement {
                    text: qsTr('Box Bottom Right')
                    value: '%luma19.pgm'
                }

                ListElement {
                    text: qsTr('Box Bottom Left')
                    value: '%luma20.pgm'
                }

                ListElement {
                    text: qsTr('Box Right Center')
                    value: '%luma21.pgm'
                }

                ListElement {
                    text: qsTr('Clock Top')
                    value: '%luma22.pgm'
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                resourceCombo.currentIndex = 0;
                resourceCombo.updateResource(resourceCombo.currentIndex);
            }
        }

        Item {
            width: 1
        }

        Shotcut.Button {
            id: favoriteButton

            visible: fileLabel.text.length > 0
            icon.name: 'bookmarks'
            icon.source: 'qrc:///icons/oxygen/32x32/places/bookmarks.png'
            implicitWidth: 32
            Layout.alignment: Qt.AlignRight
            onClicked: {
                application.addWipe(shapeFile.filePath);
            }
        }

        Item {
            width: 1
            visible: !favoriteButton.visible
        }

        Label {
            id: fileLabel

            Layout.columnSpan: 3

            Shotcut.HoverTip {
                id: fileLabelTip
            }
        }

        Item {
            width: 1
        }

        RowLayout {
            Layout.columnSpan: 3

            CheckBox {
                id: invertCheckBox

                text: qsTr('Invert')
                onClicked: filter.set('filter.invert', checked)
            }

            Shotcut.UndoButton {
                onClicked: {
                    invertCheckBox.checked = false;
                    filter.set('filter.invert', 0);
                }
            }

            Item {
                width: 1
            }

            CheckBox {
                // reset the old reverse
                id: reverseCheckBox

                text: qsTr('Reverse')
                visible: filter.isAtLeastVersion(2)
                onClicked: {
                    filter.set(reverseProperty, checked);
                    if (filter.isAtLeastVersion(3))
                        filter.set('filter.reverse', 0);
                }
            }

            Shotcut.UndoButton {

                // reset the old reverse
                visible: reverseCheckBox.visible
                onClicked: {
                    reverseCheckBox.checked = false;
                    filter.set(reverseProperty, checked);
                    if (filter.isAtLeastVersion(3))
                        filter.set('filter.reverse', 0);
                }
            }

            Item {
                width: 1
            }
        }

        Label {
            text: qsTr('Channel')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            ButtonGroup {
                id: channelGroup
            }

            RadioButton {
                id: brightnessRadioButton

                text: qsTr('Brightness')
                ButtonGroup.group: channelGroup
                onClicked: filter.set('filter.use_luminance', 1)
            }

            RadioButton {
                id: alphaRadioButton

                text: qsTr('Alpha')
                ButtonGroup.group: channelGroup
                onClicked: filter.set('filter.use_luminance', 0)
            }

            Shotcut.ComboBox {
                id: operationCombo

                implicitWidth: 180
                textRole: 'text'
                visible: filter.isAtLeastVersion(4) && alphaRadioButton.checked && !thresholdCheckBox.checked
                onActivated: filter.set('filter.alpha_operation', operationModel.get(currentIndex).value)

                model: ListModel {
                    id: operationModel

                    ListElement {
                        text: qsTr('Overwrite')
                        value: 'overwrite'
                    }

                    ListElement {
                        text: qsTr('Maximum')
                        value: 'maximum'
                    }

                    ListElement {
                        text: qsTr('Minimum')
                        value: 'minimum'
                    }

                    ListElement {
                        text: qsTr('Add')
                        value: 'add'
                    }

                    ListElement {
                        text: qsTr('Subtract')
                        value: 'subtract'
                    }
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                brightnessRadioButton.checked = true;
                operationCombo.currentIndex = 0;
                filter.set('filter.alpha_operation', "overwrite");
            }
        }

        Item {
            width: 1
        }

        CheckBox {
            id: thresholdCheckBox

            text: qsTr('Threshold')
            Layout.alignment: Qt.AlignRight
            onClicked: filter.set('filter.use_mix', checked)
        }

        Shotcut.SliderSpinner {
            id: thresholdSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter('filter.mix', value, getPosition(), thresholdKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: thresholdSlider.value = 50
        }

        Shotcut.KeyframesButton {
            id: thresholdKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, 'filter.mix', thresholdSlider.value)
        }

        Label {
            text: qsTr('Softness')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: softnessSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            onValueChanged: filter.set('filter.softness', value / 100)
        }

        Shotcut.UndoButton {
            onClicked: softnessSlider.value = 0
        }

        Item {
            width: 1
        }

        Shotcut.TipBox {
            Layout.columnSpan: parent.columns
            Layout.fillWidth: true
            Layout.margins: 10
            text: qsTr('Tip: Mask other video filters by adding filters after this one followed by <b>Mask: Apply</b>')
        }

        Label {
            Layout.fillHeight: true
        }
    }

    Connections {
        function onChanged() {
            setControls();
        }

        function onInChanged() {
            updatedSimpleAnimation();
        }

        function onOutChanged() {
            updatedSimpleAnimation();
        }

        function onAnimateInChanged() {
            updatedSimpleAnimation();
        }

        function onAnimateOutChanged() {
            updatedSimpleAnimation();
        }

        function onPropertyChanged(name) {
            setControls();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            setKeyframedControls();
        }

        target: producer
    }
}
