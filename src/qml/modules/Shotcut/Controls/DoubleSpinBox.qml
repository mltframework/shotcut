/*
 * Copyright (c) 2021 Meltytech, LLC
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

Item {
    id: root
    property real value: 0.0
    property int decimals: 0
    property real from: 0.0
    property real to: 100.0
    property real stepSize: 1.0
    property alias prefix: prefixText.text
    property alias suffix: suffixText.text
    property alias focusPolicy: spinbox.focusPolicy
    property alias background: spinbox.background
    property alias up: spinbox.up
    property alias down: spinbox.down
    property alias verticalPadding: spinbox.verticalPadding
    property alias horizontalPadding: spinbox.horizontalPadding
    property alias horizontalAlignment: textInput.horizontalAlignment
    property real _factor: Math.pow(10, decimals)
    property bool _blockSpinUpdate: false
    property bool _blockTextUpdate: false

    implicitHeight: spinbox.implicitHeight

    Component.onCompleted: {
        updateValues()
    }

    onValueChanged: {
        updateValues()
    }

    function updateValues() {
        if (!_blockSpinUpdate)
        {
            spinbox.value = value * _factor
        }
        if (!_blockTextUpdate)
        {
            textInput.text = spinbox.textFromValue(spinbox.value, spinbox.locale)
        }
    }

    SpinBox{
        id: spinbox
        stepSize: parent.stepSize * _factor
        to : parent.to * _factor
        from : parent.from * _factor
        editable: true
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        wheelEnabled: true
        anchors.fill: parent

        contentItem: RowLayout{
            Text{
                id: prefixText
                font: textInput.font
                color: textInput.color
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                visible: text != ""
            }
            TextInput {
                id: textInput
                Layout.fillWidth: true
                font: spinbox.font
                selectByMouse: true
                color: spinbox.palette.text
                selectedTextColor: spinbox.palette.highlightedText
                selectionColor : spinbox.palette.highlight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                property var _lastValidText: ""
                inputMethodHints: Qt.ImhFormattedNumbersOnly

                function validNumberFormat(text, locale) {
                    var decimalSplit = text.split(locale.decimalPoint)
                    // Empty string is ok - editing in progress
                    if (!text) {
                        return true
                    }
                    // Multiple commas
                    else if (text.split(locale.groupSeparator).length > 2) {
                        return false
                    }
                    // Multiple minus signs
                    else if (text.split(locale.negativeSign).length > 2) {
                        return false
                    }
                    // Multiple plus sign
                    else if (text.split(locale.plusSign).length > 2) {
                        return false
                    }
                    // Multiple decimal points
                    else if (decimalSplit.length > 2) {
                        return false
                    }
                    // Too many decimals
                    else if (decimalSplit.length == 2 && decimalSplit[1].length > root.decimals) {
                        return false
                    }
                    // Leading zeros
                    else if (decimalSplit[0].length > 1 && decimalSplit[0].startsWith("0")) {
                        return false
                    }
                    return true
                }

                function textInProgress(text, locale) {
                    if (!text || text == locale.negativeSign || text == locale.decimalPoint) {
                        return true
                    }
                    return false
                }

                onTextEdited: {
                    _blockTextUpdate = true
                    if (!validNumberFormat(text, spinbox.locale)) {
                        text = _lastValidText
                    }
                    else if (textInProgress(text, spinbox.locale)) {
                        // Do not parse - allow editing to continue
                    }
                    else if (isNaN(text.replace(',', '').replace('.', ''))) {
                        // Reject non-numbers
                        text = _lastValidText
                    }
                    else {
                        var newValue = spinbox.valueFromText(text, spinbox.locale)
                        if (isNaN(newValue)) {
                           // Assume editing in progress
                        }
                        else if (newValue >= spinbox.from && newValue <= spinbox.to)
                        {
                            _lastValidText = text
                            spinbox.value = newValue
                        }
                        else if (text.length > 2)
                        {
                            // Forbid out of bounds text except short text that might be in progress
                            text = _lastValidText
                        }
                    } 
                    _blockTextUpdate = false
                }
                onTextChanged: {
                    if (!_blockTextUpdate) _lastValidText = text
                }
                onEditingFinished: {
                    _blockTextUpdate = true
                    // Reformat the text to fill in missing decimals or replace invalid text
                    text = spinbox.textFromValue(spinbox.value, spinbox.locale)
                    _blockTextUpdate = false
                }
            }
            Text{
                id: suffixText
                font: textInput.font
                color: textInput.color
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                visible: text != ""
            }
        }

        onValueChanged: {
            _blockSpinUpdate = true
            root.value = value * 1.0 / _factor
            _blockSpinUpdate = false
        }

        textFromValue: function(value, locale) {
            var realValue = value * 1.0 / _factor
            return Number(realValue).toLocaleString(locale, 'f', decimals)
        }

        valueFromText: function(text, locale) {
            var textValue = Number.fromLocaleString(locale, text)
            return textValue * 1.0 * _factor
        }
    }
}
