/*
 * Copyright (c) 2025 Meltytech, LLC
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
    function setComboIndex(combo, current) {
        for (let i = 0; i < combo.model.count; ++i) {
            if (combo.model.get(i).value === current) {
                combo.currentIndex = i;
                break;
            }
        }
    }

    function setControls() {
        textArea.text = filter.get('argument');
        textFilterUi.setControls();
        stepLengthSpinner.value = filter.getDouble('typewriter.step_length');
        stepSigmaSpinner.value = filter.getDouble('typewriter.step_sigma');
        setComboIndex(macroTypeCombo, filter.getDouble('typewriter.macro_type'));
        setComboIndex(cursorCombo, filter.getDouble('typewriter.cursor'));
        setComboIndex(cursorCharCombo, filter.get('typewriter.cursor_char'));
        if (cursorCharCombo.currentIndex === -1)
            cursorCharCombo.editText = filter.get('typewriter.cursor_char');
        cursorBlinkRateSpinner.value = filter.getDouble('typewriter.cursor_blink_rate');
    }

    keyframableParameters: ['fgcolour', 'olcolour', 'bgcolour', 'opacity']
    startValues: [Qt.rgba(0, 1, 0, 1), Qt.rgba(0, 0, 0, 2.0 / 3.0), Qt.rgba(0, 0, 0, 0), 0.0]
    middleValues: [Qt.rgba(0, 1, 0, 1), Qt.rgba(0, 0, 0, 2.0 / 3.0), Qt.rgba(0, 0, 0, 0), 1.0]
    endValues: [Qt.rgba(0, 1, 0, 1), Qt.rgba(0, 0, 0, 2.0 / 3.0), Qt.rgba(0, 0, 0, 0), 0.0]
    width: 425
    height: 570
    Component.onCompleted: {
        filter.blockSignals = true;
        filter.set(textFilterUi.middleValue, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(textFilterUi.startValue, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(textFilterUi.endValue, Qt.rect(0, 0, profile.width, profile.height));
        if (filter.isNew) {
            var presetParams = preset.parameters.slice();
            var index = presetParams.indexOf('argument');
            if (index > -1)
                presetParams.splice(index, 1);
            filter.set('argument', '');
            // Set monospace font for typewriter effect
            if (application.OS === 'Windows')
                filter.set('family', 'Consolas');
            else if (application.OS === 'macOS')
                filter.set('family', "Monaco");
            else
                filter.set('family', 'monospace');

            filter.set('fgcolour', Qt.color('#ff00ff00')); // Green terminal text
            filter.set('bgcolour', Qt.color('#00000000'));
            filter.set('olcolour', Qt.color('#aa000000'));
            filter.set('opacity', 1.0);
            filter.set('outline', 0);
            filter.set('weight', Font.Normal);
            filter.set('style', 'normal');
            filter.set(textFilterUi.useFontSizeProperty, 1);
            filter.set('size', profile.height / 15); // Smaller default size
            filter.set('typewriter', 1);
            filter.set('typewriter.step_length', 8);
            filter.set('typewriter.step_sigma', 2);
            filter.set('typewriter.random_seed', 0);
            filter.set('typewriter.macro_type', 1);
            filter.set('typewriter.cursor', 1);
            filter.set('typewriter.cursor_blink_rate', 25);
            filter.set('typewriter.cursor_char', '|');

            filter.set(textFilterUi.rectProperty, '5%/5%:90%x90%');
            filter.set(textFilterUi.valignProperty, 'bottom');
            filter.set(textFilterUi.halignProperty, 'center');
            filter.savePreset(presetParams, qsTr('Lower Third'));

            // Add some typewriter-specific presets
            filter.set(textFilterUi.valignProperty, 'top');
            filter.set(textFilterUi.halignProperty, 'left');
            filter.set('typewriter.step_length', 5);
            filter.set('typewriter.cursor_char', '█');
            filter.savePreset(presetParams, qsTr('Terminal Style'));
            filter.set('typewriter.step_length', 15);
            filter.set('typewriter.macro_type', 2);
            filter.set('typewriter.cursor_char', '_');
            filter.savePreset(presetParams, qsTr('Word by Word'));
            filter.set('typewriter.step_length', 50);
            filter.set('typewriter.macro_type', 3);
            filter.set('typewriter.cursor', 0);
            filter.savePreset(presetParams, qsTr('Line by Line'));
            // Reset to defaults
            filter.set(textFilterUi.halignProperty, 'center');
            filter.set(textFilterUi.valignProperty, 'middle');
            filter.set('typewriter.step_length', 8);
            filter.set('typewriter.macro_type', 1);
            filter.set('typewriter.cursor', 1);
            filter.set('typewriter.cursor_char', '|');
            filter.set(textFilterUi.rectProperty, '0%/0%:100%x100%');
            filter.savePreset(presetParams);
        } else {
            if (filter.get('opacity') === null)
                filter.set('opacity', 1.0);
            // Ensure typewriter is always enabled
            filter.set('typewriter', 1);
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
            parameters: textFilterUi.parameterList.concat(['typewriter.step_length', 'typewriter.step_sigma', 'typewriter.random_seed', 'typewriter.macro_type', 'typewriter.cursor', 'typewriter.cursor_blink_rate', 'typewriter.cursor_char', 'argument'])
            onBeforePresetLoaded: {
                filter.resetProperty(textFilterUi.rectProperty);
                filter.set(textFilterUi.pointSizeProperty, 0);
                resetSimpleKeyframes();
            }
            onPresetSelected: {
                if (filter.get('opacity') === '')
                    filter.set('opacity', 1.0);
                // Ensure typewriter stays enabled
                filter.set('typewriter', 1);
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
            text: qsTr('Text')
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
        }

        Item {
            Layout.columnSpan: 3
            Layout.minimumHeight: fontMetrics.height * 6
            Layout.maximumHeight: Layout.minimumHeight
            Layout.minimumWidth: preset.width
            Layout.maximumWidth: preset.width

            FontMetrics {
                id: fontMetrics

                font: textArea.font
            }

            ScrollView {
                id: scrollview

                width: preset.width - (ScrollBar.vertical.visible ? 16 : 0)
                height: parent.height - (ScrollBar.horizontal.visible ? 16 : 0)
                clip: true

                TextArea {
                    id: textArea

                    property int maxLength: 256

                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.NoWrap
                    selectByMouse: true
                    persistentSelection: true
                    padding: 0
                    text: '__empty__'
                    onTextChanged: {
                        if (text === '__empty__')
                            return;
                        if (length > maxLength) {
                            text = text.substring(0, maxLength);
                            cursorPosition = maxLength;
                        }
                        if (!parseInt(filter.get(textFilterUi.useFontSizeProperty)))
                            filter.set('size', profile.height / text.split('\n').length);
                        filter.set('argument', text);
                    }
                    Keys.onPressed: event => {
                        if (event.key === Qt.Key_V && (event.modifiers & Qt.ShiftModifier) && (event.modifiers & Qt.ControlModifier || event.modifiers & Qt.MetaModifier)) {
                            event.accepted = true;
                            textArea.paste();
                        }
                    }

                    MouseArea {
                        acceptedButtons: Qt.RightButton
                        anchors.fill: parent
                        onClicked: contextMenu.popup()
                    }

                    background: Rectangle {
                        anchors.fill: parent
                        color: textArea.palette.base
                    }

                    Shotcut.EditMenu {
                        id: contextMenu
                    }
                }
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

        Item {
            Layout.fillWidth: true
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

        Item {
            Layout.fillWidth: true
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
            textRole: 'text'
            valueRole: 'value'
            onActivated: filter.set('typewriter.macro_type', model.get(currentIndex).value)
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set('typewriter.macro_type', 1);
                macroTypeCombo.currentIndex = 0;
            }
        }

        Item {
            Layout.fillWidth: true
        }

        // Cursor Settings
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
            textRole: 'text'
            valueRole: 'value'
            onActivated: filter.set('typewriter.cursor', currentValue)
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set('typewriter.cursor', 1);
                cursorCombo.currentIndex = 1;
            }
        }

        Item {
            Layout.fillWidth: true
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
            textRole: 'text'
            valueRole: 'value'
            onActivated: filter.set('typewriter.cursor_char', currentValue)
            onEditTextChanged: {
                if (editText.length > 0) {
                    filter.set('typewriter.cursor_char', editText.charAt(0));
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set('typewriter.cursor_char', '|');
                cursorCharCombo.currentIndex = 0;
            }
        }

        Item {
            Layout.fillWidth: true
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

        Item {
            Layout.fillWidth: true
        }

        Shotcut.TextFilterUi {
            id: textFilterUi
            Layout.columnSpan: 4
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
