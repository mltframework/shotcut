/*
 * Copyright (c) 2024-2026 Meltytech, LLC
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
    id: root

    property bool _typewriterExpanded: false

    function setComboIndex(combo, current) {
        for (let i = 0; i < combo.model.count; ++i) {
            if (combo.model.get(i).value === current) {
                combo.currentIndex = i;
                break;
            }
        }
    }

    function setControls() {
        var feedIndex = feedCombo.find(filter.get("feed"));
        feedCombo.currentIndex = feedIndex;
        textFilterUi.setControls();
        if (filter.isAtLeastVersion('4')) {
            typewriterEnabledCheckBox.checked = filter.getDouble('typewriter') > 0;
            setComboIndex(macroTypeCombo, filter.getDouble('typewriter.macro_type'));
            stepLengthSpinner.value = filter.getDouble('typewriter.step_length');
            stepSigmaSpinner.value = filter.getDouble('typewriter.step_sigma');
            setComboIndex(cursorCombo, filter.getDouble('typewriter.cursor'));
            setComboIndex(cursorCharCombo, filter.get('typewriter.cursor_char'));
            if (cursorCharCombo.currentIndex === -1)
                cursorCharCombo.editText = filter.get('typewriter.cursor_char');
            cursorBlinkRateSpinner.value = filter.getDouble('typewriter.cursor_blink_rate');
        }
    }

    keyframableParameters: ['fgcolour', 'olcolour', 'bgcolour', 'opacity']
    startValues: [Qt.rgba(1, 1, 1, 1), Qt.rgba(0, 0, 0, 2.0 / 3.0), Qt.rgba(0, 0, 0, 0), 0.0]
    middleValues: [Qt.rgba(1, 1, 1, 1), Qt.rgba(0, 0, 0, 2.0 / 3.0), Qt.rgba(0, 0, 0, 0), 1.0]
    endValues: [Qt.rgba(1, 1, 1, 1), Qt.rgba(0, 0, 0, 2.0 / 3.0), Qt.rgba(0, 0, 0, 0), 0.0]
    width: 425
    height: !filter.isAtLeastVersion('4') ? 330 : (_typewriterExpanded ? 625 : 370)

    SystemPalette {
        id: activePalette
    }
    Component.onCompleted: {
        filter.blockSignals = true;
        filter.set(textFilterUi.middleValue, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(textFilterUi.startValue, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(textFilterUi.endValue, Qt.rect(0, 0, profile.width, profile.height));
        if (filter.isNew) {
            if (application.OS === 'Windows')
                filter.set('family', 'Verdana');
            else if (application.OS === 'macOS')
                filter.set('family', 'Helvetica');
            filter.set('fgcolour', '#ffffffff');
            filter.set('bgcolour', '#00000000');
            filter.set('olcolour', '#aa000000');
            filter.set('opacity', 1.0);
            filter.set('outline', 3);
            filter.set('weight', 700);
            filter.set('style', 'normal');
            filter.set(textFilterUi.useFontSizeProperty, 1);
            filter.set('size', profile.height / 20);
            filter.set(textFilterUi.rectProperty, '20%/75%:60%x20%');
            filter.set(textFilterUi.valignProperty, 'bottom');
            filter.set(textFilterUi.halignProperty, 'center');
            filter.set('feed', feedCombo.textAt(0));
            filter.set('typewriter', 0);
            filter.set('typewriter.step_length', 15);
            filter.set('typewriter.step_sigma', 2);
            filter.set('typewriter.random_seed', 0);
            filter.set('typewriter.macro_type', 2);
            filter.set('typewriter.cursor', 0);
            filter.set('typewriter.cursor_blink_rate', 25);
            filter.set('typewriter.cursor_char', '|');
            filter.savePreset(preset.parameters);
        } else {
            if (filter.get('opacity') === null)
                filter.set('opacity', 1.0);
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

            parameters: textFilterUi.parameterList.concat(['feed', 'typewriter', 'typewriter.step_length', 'typewriter.step_sigma', 'typewriter.random_seed', 'typewriter.macro_type', 'typewriter.cursor', 'typewriter.cursor_blink_rate', 'typewriter.cursor_char'])
            onBeforePresetLoaded: {
                filter.resetProperty(textFilterUi.rectProperty);
                filter.set(textFilterUi.pointSizeProperty, 0);
                resetSimpleKeyframes();
            }
            onPresetSelected: {
                if (filter.get('opacity') === '')
                    filter.set('opacity', 1.0);
                setControls();
                textFilterUi.setKeyframedControls();
                initializeSimpleKeyframes();
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
            text: qsTr('Subtitle Track')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: feedCombo

            implicitWidth: 220
            textRole: 'text'
            onActivated: {
                if (feedCombo.currentIndex >= 0) {
                    filter.set("feed", feedCombo.currentText);
                }
            }

            model: subtitlesModel
        }

        // Typewriter section header
        Item {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            implicitHeight: 30
            visible: filter.isAtLeastVersion('4')

            RowLayout {
                anchors.fill: parent
                spacing: 6

                Label {
                    text: _typewriterExpanded ? '▼' : '►'
                    font.bold: true
                }

                Label {
                    text: qsTr('Typewriter')
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: activePalette.mid
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root._typewriterExpanded = !root._typewriterExpanded
            }
        }

        // Typewriter section body
        Item {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            visible: filter.isAtLeastVersion('4') && _typewriterExpanded
            height: visible ? typewriterGrid.implicitHeight : 0

            GridLayout {
                id: typewriterGrid

                columns: 3
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                CheckBox {
                    id: typewriterEnabledCheckBox

                    Layout.columnSpan: 2
                    text: qsTr('Enable typewriter animation')
                    onClicked: filter.set('typewriter', checked ? 1 : 0)
                }

                Shotcut.UndoButton {
                    onClicked: {
                        filter.set('typewriter', 0);
                        typewriterEnabledCheckBox.checked = false;
                    }
                }

                Label {
                    text: qsTr('Animation')
                    Layout.alignment: Qt.AlignRight

                    Shotcut.HoverTip {
                        text: qsTr('How text appears: character by character, word by word, or line by line.')
                    }
                }

                Shotcut.ComboBox {
                    id: macroTypeCombo

                    Layout.minimumWidth: 200
                    textRole: 'text'
                    valueRole: 'value'
                    onActivated: filter.set('typewriter.macro_type', model.get(currentIndex).value)

                    model: ListModel {
                        ListElement {
                            text: qsTr('Character by Character')
                            value: 1
                        }

                        ListElement {
                            text: qsTr('Word by Word')
                            value: 2
                        }

                        ListElement {
                            text: qsTr('Line by Line')
                            value: 3
                        }
                    }
                }

                Shotcut.UndoButton {
                    onClicked: {
                        filter.set('typewriter.macro_type', 2);
                        macroTypeCombo.currentIndex = 1;
                    }
                }

                Label {
                    text: qsTr('Typewriter rate')
                    Layout.alignment: Qt.AlignRight

                    Shotcut.HoverTip {
                        text: qsTr('Number of frames between each character, word, or line appearance.')
                    }
                }

                Shotcut.DoubleSpinBox {
                    id: stepLengthSpinner

                    Layout.minimumWidth: 50
                    from: 1
                    to: 1000
                    onValueModified: filter.set('typewriter.step_length', value)
                }

                Shotcut.UndoButton {
                    onClicked: {
                        filter.set('typewriter.step_length', 8);
                        stepLengthSpinner.value = 8;
                    }
                }

                Label {
                    text: qsTr('Rate variation')
                    Layout.alignment: Qt.AlignRight

                    Shotcut.HoverTip {
                        text: qsTr('Random variation in timing (0 = no variation).')
                    }
                }

                Shotcut.DoubleSpinBox {
                    id: stepSigmaSpinner

                    Layout.minimumWidth: 50
                    from: 0
                    to: 100
                    onValueModified: filter.set('typewriter.step_sigma', value)
                }

                Shotcut.UndoButton {
                    onClicked: {
                        filter.set('typewriter.step_sigma', 2);
                        stepSigmaSpinner.value = 2;
                    }
                }

                Label {
                    text: qsTr('Cursor visibility')
                    Layout.alignment: Qt.AlignRight

                    Shotcut.HoverTip {
                        text: qsTr('When to show the blinking cursor.')
                    }
                }

                Shotcut.ComboBox {
                    id: cursorCombo

                    Layout.minimumWidth: 150
                    textRole: 'text'
                    valueRole: 'value'
                    onActivated: filter.set('typewriter.cursor', currentValue)

                    model: ListModel {
                        ListElement {
                            text: qsTr('No Cursor')
                            value: 0
                        }

                        ListElement {
                            text: qsTr('While Typing')
                            value: 1
                        }

                        ListElement {
                            text: qsTr('Always Visible')
                            value: 2
                        }
                    }
                }

                Shotcut.UndoButton {
                    onClicked: {
                        filter.set('typewriter.cursor', 1);
                        cursorCombo.currentIndex = 1;
                    }
                }

                Label {
                    text: qsTr('Cursor shape')
                    Layout.alignment: Qt.AlignRight

                    Shotcut.HoverTip {
                        text: qsTr('Character to use for the blinking cursor.')
                    }
                }

                Shotcut.ComboBox {
                    id: cursorCharCombo

                    Layout.minimumWidth: 150
                    editable: true
                    currentIndex: -1
                    textRole: 'text'
                    valueRole: 'value'
                    onActivated: filter.set('typewriter.cursor_char', currentValue)
                    onEditTextChanged: {
                        if (editText.length > 0)
                            filter.set('typewriter.cursor_char', editText.charAt(0));
                    }

                    model: ListModel {
                        ListElement {
                            text: qsTr('| (Vertical Line)')
                            value: '|'
                        }

                        ListElement {
                            text: qsTr('_ (Underscore)')
                            value: '_'
                        }

                        ListElement {
                            text: qsTr('█ (Block)')
                            value: '█'
                        }

                        ListElement {
                            text: qsTr('▌ (Half Block)')
                            value: '▌'
                        }

                        ListElement {
                            text: qsTr('▊ (Thick Line)')
                            value: '▊'
                        }

                        ListElement {
                            text: qsTr('● (Bullet)')
                            value: '●'
                        }

                        ListElement {
                            text: qsTr('◆ (Diamond)')
                            value: '◆'
                        }

                        ListElement {
                            text: qsTr('► (Arrow)')
                            value: '►'
                        }
                    }
                }

                Shotcut.UndoButton {
                    onClicked: {
                        filter.set('typewriter.cursor_char', '|');
                        cursorCharCombo.currentIndex = 0;
                    }
                }

                Label {
                    text: qsTr('Blink rate')
                    Layout.alignment: Qt.AlignRight

                    Shotcut.HoverTip {
                        text: qsTr('Number of frames for cursor blink cycle.')
                    }
                }

                Shotcut.DoubleSpinBox {
                    id: cursorBlinkRateSpinner

                    Layout.minimumWidth: 150
                    from: 0
                    to: 200
                    suffix: qsTr(' frames')
                    onValueModified: filter.set('typewriter.cursor_blink_rate', value)
                }

                Shotcut.UndoButton {
                    onClicked: {
                        filter.set('typewriter.cursor_blink_rate', 25);
                        cursorBlinkRateSpinner.value = 25;
                    }
                }
            }
        }

        Shotcut.TextFilterUi {
            id: textFilterUi
            showOpacity: true
            Layout.columnSpan: 2
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
