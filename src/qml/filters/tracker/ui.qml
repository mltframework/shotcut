/*
 * Copyright (c) 2023 Meltytech, LLC
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
    property string rectProperty: 'rect'
    property rect filterRect

    function setStatus(inProgress) {
        if (inProgress) {
            status.text = qsTr('Analyzing...');
        } else if (filter.get('results').length > 0) {
            status.text = qsTr('Analysis complete.');
        } else {
            status.text = qsTr('Click "Analyze" to use this filter.');
        }
    }

    function setFilter() {
        filter.set(rectProperty, filterRect);
    }

    function setControls() {
        var newValue = filter.getRect(rectProperty);
        if (filterRect !== newValue) {
            filterRect = newValue;
            rectX.value = filterRect.x.toFixed();
            rectY.value = filterRect.y.toFixed();
            rectW.value = filterRect.width.toFixed();
            rectH.value = filterRect.height.toFixed();
        }
        algorithmCombo.currentIndex = algorithmCombo.indexOfValue(filter.get('algo'));
        algorithmCombo.previousIndex = algorithmCombo.currentIndex;
        previewCheckBox.checked = parseInt(filter.get('shape_width')) !== 0;
    }

    function visibleShapeWidth() {
        return Math.ceil(Math.max(profile.width, profile.height) / 640);
    }

    width: 450
    height: 200
    Component.onCompleted: {
        if (filter.isNew) {
            // Add default preset.
            filter.set(rectProperty, '45%/45%:10%x10%');
            filter.set('algo', 'KCF');
            filter.savePreset(preset.parameters);
            filter.set('shape_width', visibleShapeWidth);
            filter.set('shape_color', '#00ff00');
            filter.set('modelsfolder', settings.appDataLocation + '/opencvmodels');
        }
        setStatus(false);
        setControls();
        if (filter.isNew)
            filter.set(rectProperty, filter.getRect(rectProperty));
    }

    Connections {
        function onAnalyzeFinished() {
            setStatus(false);
            button.enabled = true;
        }

        function onChanged() {
            setControls();
        }

        target: filter
    }

    Shotcut.File {
        id: dasiamFile
        url: settings.appDataLocation + '/opencvmodels/dasiamrpn_model.onnx'
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 3

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: [rectProperty, 'algo']
            Layout.columnSpan: 2
            onPresetSelected: {
                setControls();
            }
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {
                id: positionTip
                text: qsTr('Set the region of interest to track.')
            }
        }

        RowLayout {

            Shotcut.DoubleSpinBox {
                id: rectX

                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -1e+09
                to: 1e+09
                onValueModified: {
                    if (filterRect.x !== value) {
                        filterRect.x = value;
                        setFilter();
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
                from: -1e+09
                to: 1e+09
                onValueModified: {
                    if (filterRect.y !== value) {
                        filterRect.y = value;
                        setFilter();
                    }
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                rectX.value = rectY.value = 0;
                filterRect.x = filterRect.y = 0;
                setFilter();
            }
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: positionTip.text
            }
        }

        RowLayout {

            Shotcut.DoubleSpinBox {
                id: rectW

                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -1e+09
                to: 1e+09
                onValueModified: {
                    if (filterRect.width !== value) {
                        filterRect.width = value;
                        setFilter();
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
                from: -1e+09
                to: 1e+09
                onValueModified: {
                    if (filterRect.height !== value) {
                        filterRect.height = value;
                        setFilter();
                    }
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                rectW.value = profile.width / 10;
                rectH.value = profile.height / 10;
                filterRect.width = profile.width / 10;
                filterRect.height = profile.height / 10;
                setFilter();
            }
        }

        Label {
            Layout.alignment: Qt.AlignRight
            text: qsTr('Algorithm')

            Shotcut.HoverTip {
                text: qsTr('Chooses the way (rules) the tracking is calculated.')
            }
        }

        Shotcut.ComboBox {
            id: algorithmCombo

            property int previousIndex: -1

            function updateFilter(index) {
                filter.set('algo', model.get(index).value);
                previousIndex = index;
            }

            implicitWidth: 400
            textRole: 'text'
            valueRole: 'value'
            currentIndex: 0

            Component.onCompleted: {
                if (dasiamFile.exists()) {
                    model.append({
                            "text": 'DaSiam: Distractor-aware Siamese Networks',
                            "value": 'DaSIAM'
                        });
                }
            }

            onActivated: {
                // toggling focus works around a weird bug involving sticky
                // input event focus on the ComboBox
                enabled = false;
                updateFilter(currentIndex);
                enabled = true;
            }

            model: ListModel {
                id: listModel

                ListElement {
                    text: 'KCF: Kernelized Correlation Filters'
                    value: 'KCF'
                }

                ListElement {
                    text: 'MIL: Multiple Instance Learning'
                    value: 'MIL'
                }

                ListElement {
                    text: 'BOOSTING: AdaBoost'
                    value: 'BOOSTING'
                }

                ListElement {
                    text: 'TLD: Tracking Learning Detection'
                    value: 'TLD'
                }

                ListElement {
                    text: 'MEDIANFLOW: Lucas-Kanade Median Flow'
                    value: 'MEDIANFLOW'
                }

                ListElement {
                    text: 'GOTURN: Regression Network'
                    value: 'GOTURN'
                }

                ListElement {
                    text: 'MOSSE: Minimum Output Sum of Squared Error'
                    value: 'MOSSE'
                }

                ListElement {
                    text: 'CSRT: Discriminative Correlation Filter with Channel and Spatial Reliability'
                    value: 'CSRT'
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                algorithmCombo.currentIndex = 0;
                algorithmCombo.updateFilter(0);
            }
        }

        Label {
        }

        CheckBox {
            id: previewCheckBox

            text: qsTr('Show preview')
            Layout.columnSpan: 2
            onClicked: filter.set('shape_width', checked ? visibleShapeWidth() : 0)
        }

        Label {
        }

        Shotcut.Button {
            id: button

            text: qsTr('Analyze')
            Layout.columnSpan: 2
            onClicked: {
                button.enabled = false;
                setStatus(true);
                filter.set('_reset', 1);
                filter.analyze();
            }
        }

        Rectangle {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.minimumHeight: 12
            color: 'transparent'

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 2
                radius: 2
                color: activePalette.text
            }
        }

        Label {
            id: status

            Layout.columnSpan: 3
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
