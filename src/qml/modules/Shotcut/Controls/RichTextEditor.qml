/*
 * Copyright (c) 2022 Meltytech, LLC
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
import QtQuick.Dialogs 1.2
import Shotcut.Controls 1.0 as Shotcut
import org.shotcut.qml 1.0

Item {
    id: root
    property bool blockUpdate: false
    property int minWidth: toolbar.visible ? toolbar.width : 200
    property bool allowRichText: true
    signal textModified(string text)

    property var _textFormat: Qt.PlainText

    function setText(text) {
        // Detect format
        if (text.startsWith("<!DOCTYPE HTML PUBLIC")) {
            formatRichAction.checked = true
            richTextArea.text = text
            root._textFormat = Qt.RichText
            richTextArea.visible = true
            plainTextArea.visible = false
        } else {
            formatPlainAction.checked = true
            plainTextArea.text = text
            root._textFormat = Qt.PlainText
            richTextArea.visible = false
            plainTextArea.visible = true
        }
    }

    function getText() {
        if (root._textFormat == Qt.PlainText) {
            return plainTextArea.text
        } else {
            return richTextArea.text
        }
    }

    function textArea() {
        if (root._textFormat == Qt.PlainText) {
            return plainTextArea
        } else {
            return richTextArea
        }
    }

    ToolBar {
        id: toolbar
        visible: allowRichText
        RowLayout {
            Shotcut.ToolButton {
                Shotcut.HoverTip { text: qsTr('Menu') }
                implicitHeight: settings.smallIcons ? 22 : 30
                implicitWidth: height
                display: AbstractButton.IconOnly
                focusPolicy: Qt.NoFocus
                action: Action {
                    icon.name: 'show-menu'
                    icon.source: 'qrc:///icons/oxygen/32x32/actions/show-menu.png'
                    onTriggered: menu.popup()
                }
            }
            Shotcut.ToolButton {
                action: boldAction
                implicitHeight: settings.smallIcons ? 22 : 30
                implicitWidth: height
                display: AbstractButton.IconOnly
                focusPolicy: Qt.NoFocus
                enabled: root._textFormat == Qt.RichText
                Shotcut.HoverTip { text: parent.action.text }
            }
            Shotcut.ToolButton {
                action: italicAction
                implicitHeight: settings.smallIcons ? 22 : 30
                implicitWidth: height
                display: AbstractButton.IconOnly
                enabled: root._textFormat == Qt.RichText
                focusPolicy: Qt.NoFocus
                Shotcut.HoverTip { text: parent.action.text }
            }
            Shotcut.ToolButton {
                action: underlineAction
                implicitHeight: settings.smallIcons ? 22 : 30
                implicitWidth: height
                display: AbstractButton.IconOnly
                enabled: root._textFormat == Qt.RichText
                focusPolicy: Qt.NoFocus
                Shotcut.HoverTip { text: parent.action.text }
            }
            Button { // separator
                enabled: false
                implicitWidth: 2
                implicitHeight: toolbar.height / 2
            }
            Shotcut.ToolButton {
                Shotcut.HoverTip { text: qsTr('Font') }
                implicitHeight: settings.smallIcons ? 22 : 30
                implicitWidth: height
                display: AbstractButton.IconOnly
                focusPolicy: Qt.NoFocus
                enabled: root._textFormat == Qt.RichText
                action: Action {
                    icon.name: 'font'
                    icon.source: 'qrc:///icons/oxygen/32x32/actions/font.png'
                    onTriggered: {
                        fontDialog.font.family = document.fontFamily
                        fontDialog.font.pointSize = document.fontSize
                        fontDialog.open()
                    }
                }
            }
            Shotcut.DoubleSpinBox {
                id: fontSizeSpinBox
                enabled: root._textFormat == Qt.RichText
                Shotcut.HoverTip { text: qsTr('Text size') }
                implicitWidth: 60
                value: 12
                from: 1
                to: 1000
                decimals: 0
                focusPolicy: Qt.NoFocus
                property bool blockValue: false
                onValueChanged: {
                    if (!blockValue) {
                        blockValue = true
                        document.fontSize = value
                        blockValue = false
                    }
                }
            }
            Shotcut.ToolButton {
                id: colorButton
                Shotcut.HoverTip { text: qsTr('Text color') }
                implicitHeight: settings.smallIcons ? 22 : 30
                implicitWidth: implicitHeight
                focusPolicy: Qt.NoFocus
                enabled: root._textFormat == Qt.RichText
                property var color : document.textColor
                Rectangle {
                    id: colorRect
                    anchors.fill: parent
                    anchors.margins: 4
                    height: parent.height - 4
                    width: parent.height -  4
                    color: Qt.darker(document.textColor, colorButton.pressed ? 1.4 : 1)
                    border.width: 1
                    border.color: Qt.darker(colorRect.color, 2)
                }
                onClicked: {
                    colorDialog.color = document.textColor
                    colorDialog.open()
                }
            }
            Button { // separator
                enabled: false
                implicitWidth: 2
                implicitHeight: toolbar.height / 2
            }
            Shotcut.ToolButton {
                action: alignLeftAction
                implicitHeight: settings.smallIcons ? 22 : 30
                implicitWidth: height
                display: AbstractButton.IconOnly
                focusPolicy: Qt.NoFocus
                enabled: root._textFormat == Qt.RichText
                Shotcut.HoverTip { text: parent.action.text }
            }
            Shotcut.ToolButton {
                action: alignCenterAction
                implicitHeight: settings.smallIcons ? 22 : 30
                implicitWidth: height
                display: AbstractButton.IconOnly
                focusPolicy: Qt.NoFocus
                enabled: root._textFormat == Qt.RichText
                Shotcut.HoverTip { text: parent.action.text }
            }
            Shotcut.ToolButton {
                action: alignRightAction
                implicitHeight: settings.smallIcons ? 22 : 30
                implicitWidth: height
                display: AbstractButton.IconOnly
                focusPolicy: Qt.NoFocus
                enabled: root._textFormat == Qt.RichText
                Shotcut.HoverTip { text: parent.action.text }
            }
            Shotcut.ToolButton {
                action: alignJustifyAction
                implicitHeight: settings.smallIcons ? 22 : 30
                implicitWidth: height
                display: AbstractButton.IconOnly
                focusPolicy: Qt.NoFocus
                enabled: root._textFormat == Qt.RichText
                Shotcut.HoverTip { text: parent.action.text }
            }
            Shotcut.ToolButton {
                action: decreaseIndentAction
                implicitHeight: settings.smallIcons ? 22 : 30
                implicitWidth: height
                display: AbstractButton.IconOnly
                focusPolicy: Qt.NoFocus
                enabled: root._textFormat == Qt.RichText
                Shotcut.HoverTip { text: parent.action.text }
            }
            Shotcut.ToolButton {
                action: increaseIndentAction
                implicitHeight: settings.smallIcons ? 22 : 30
                implicitWidth: height
                display: AbstractButton.IconOnly
                focusPolicy: Qt.NoFocus
                enabled: root._textFormat == Qt.RichText
                Shotcut.HoverTip { text: parent.action.text }
            }
        }
    }

    ScrollView {
        id: scrollView
        clip: true
        anchors.top: toolbar.visible ? toolbar.bottom : parent.top
        anchors.left: parent.left
        width: parent.width - verticalScroll.width
        height: parent.height - (toolbar.visible ? toolbar.height : 0 ) - horizontalScroll.height
        contentWidth: textArea().implicitWidth
        contentHeight: textArea().implicitHeight
        SystemPalette { id: activePalette }
        ScrollBar.horizontal: ScrollBar {
            id: horizontalScroll
            height: textArea().implicitWidth > root.width ? 16 : 0
            policy: ScrollBar.AlwaysOn
            visible: textArea().implicitWidth > scrollView.width
            parent: scrollView.parent
            anchors.top: scrollView.bottom
            anchors.left: scrollView.left
            anchors.right: scrollView.right
            background: Rectangle { color: activePalette.alternateBase }
        }
        ScrollBar.vertical: ScrollBar {
            id: verticalScroll
            width: textArea().implicitHeight > (root.height - (toolbar.visible ? toolbar.height : 0 )) ? 16 : 0
            policy: ScrollBar.AlwaysOn
            visible: textArea().implicitHeight > scrollView.height
            parent: scrollView.parent
            anchors.top: scrollView.top
            anchors.left: scrollView.right
            anchors.bottom: scrollView.bottom
            background: Rectangle { color: activePalette.alternateBase }
        }
        TextArea {
            id: plainTextArea
            padding: 0
            selectByMouse: true
            persistentSelection: true
            wrapMode: TextArea.NoWrap
            baseUrl: 'qrc:/'
            textFormat: Qt.PlainText
            visible: root._textFormat == Qt.PlainText
            MouseArea {
                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                onClicked: contextMenu.popup()
            }
            onTextChanged: {
                root.textModified(text)
            }
        }
        TextArea {
            id: richTextArea
            padding: 0
            selectByMouse: true
            persistentSelection: true
            wrapMode: TextArea.NoWrap
            baseUrl: 'qrc:/'
            textFormat: Qt.RichText
            visible: root._textFormat == Qt.RichText
            MouseArea {
                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                onClicked: contextMenu.popup()
            }
            onTextChanged: {
                root.textModified(text)
            }
            Keys.onPressed: {
                if (event.key === Qt.Key_V && (event.modifiers & Qt.ShiftModifier) &&
                   (event.modifiers & Qt.ControlModifier || event.modifiers & Qt.MetaModifier)) {
                    event.accepted = true
                    document.pastePlain()
                }
            }
        }
    }

    Menu {
        id: contextMenu
        width: 220
        MenuItem { action: undoAction }
        MenuItem { action: redoAction }
        MenuSeparator {}
        MenuItem { action: cutAction }
        MenuItem { action: copyAction }
        MenuItem { action: pasteAction }
        MenuItem {
            action: pastePlainAction
            visible: root.allowRichText
            height: visible ? implicitHeight : 0
        }
        MenuItem { action: deleteAction }
        MenuItem { action: clearAction }
        MenuSeparator {}
        MenuItem { action: selectAllAction }
        MenuItem {
            text: qsTr('Cancel')
            onTriggered: contextMenu.dismiss()
        }
    }

    Menu {
        id: menu
        Menu {
            title: qsTr('Format')
            MenuItem { action: formatPlainAction }
            MenuItem { action: formatRichAction }
        }
        Menu {
            width: 220
            title: qsTr('Edit')
            MenuItem { action: undoAction }
            MenuItem { action: redoAction }
            MenuSeparator {}
            MenuItem { action: cutAction }
            MenuItem { action: copyAction }
            MenuItem { action: pasteAction }
            MenuItem {
                action: pastePlainAction
                visible: root.allowRichText
                height: visible ? implicitHeight : 0
            }
        }
        MenuItem { action: selectAllAction }
        MenuItem {
            text: qsTr('Cancel')
            onTriggered: menu.dismiss()
        }
    }

    Action {
        id: menuAction
        icon.name: 'show-menu'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/show-menu.png'
        onTriggered: menu.popup()
    }
    ActionGroup {
        id: formatGroup
        Action {
            id: formatPlainAction
            text: qsTr('Plain Text')
            checkable: true
            checked: true
            onTriggered: {
                if (root._textFormat != Qt.PlainText) {
                    plainTextArea.text = richTextArea.getText(0, richTextArea.length)
                    root._textFormat = Qt.PlainText
                    plainTextArea.visible = true
                    richTextArea.visible = false
                }
            }
        }
        Action {
            id: formatRichAction
            text: qsTr('Rich Text')
            checkable: true
            onTriggered: {
                if (root._textFormat != Qt.RichText) {
                    richTextArea.text = plainTextArea.getText(0, plainTextArea.length).replace(/\r?\n/g, "<br />")
                    root._textFormat = Qt.RichText
                    plainTextArea.visible = false
                    richTextArea.visible = true
                }
            }
        }
    }
    Action {
        id: undoAction
        text: qsTr('Undo') + (application.OS === 'OS X'? '    ⌘Z' : ' (Ctrl+Z)')
        enabled: textArea().canUndo
        onTriggered: textArea().undo()
    }
    Action {
        id: redoAction
        text: qsTr('Redo') + (application.OS === 'Windows'? ' (Ctrl+Y)' : application.OS === 'OS X'? '    ⇧⌘Z' : ' (Ctrl+Shift+Z)')
        enabled: textArea().canRedo
        onTriggered: textArea().redo()
    }
    Action {
        id: cutAction
        text: qsTr('Cut') + (application.OS === 'OS X'? '    ⌘X' : ' (Ctrl+X)')
        onTriggered: textArea().cut()
    }
    Action {
        id: copyAction
        text: qsTr('Copy') + (application.OS === 'OS X'? '    ⌘C' : ' (Ctrl+C)')
        onTriggered: textArea().copy()
    }
    Action {
        id: pasteAction
        text: qsTr('Paste') + (application.OS === 'OS X'? '    ⌘V' : ' (Ctrl+V)')
        onTriggered: textArea().paste()
    }
    Action {
        id: pastePlainAction
        text: qsTr('Paste Text Only') + (application.OS === 'OS X'? '    ⇧⌘V' : ' (Ctrl+Shift+V)')
        enabled: root._textFormat == Qt.RichText
        onTriggered: document.pastePlain()
    }
    Action {
        id: deleteAction
        text: qsTr('Delete')
        onTriggered: textArea().remove(textArea().selectionStart, textArea().selectionEnd)
    }
    Action {
        id: clearAction
        text: qsTr('Clear')
        onTriggered: {
            textArea().selectAll()
            textArea().remove(textArea().selectionStart, textArea().selectionEnd)
        }
    }
    Action {
        id: selectAllAction
        text: qsTr('Select All') + (application.OS === 'OS X'? '    ⌘A' : ' (Ctrl+A)')
        onTriggered: {
            textArea().selectAll()
        }
    }

    Action {
        id: alignLeftAction
        text: qsTr('Left')
        icon.name: 'format-justify-left'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/format-justify-left.png'
        onTriggered: document.alignment = Qt.AlignLeft
        checkable: true
        checked: document.alignment == Qt.AlignLeft
    }
    Action {
        id: alignCenterAction
        text: qsTr('Center')
        icon.name: 'format-justify-center'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/format-justify-center.png'
        onTriggered: document.alignment = Qt.AlignHCenter
        checkable: true
        checked: document.alignment == Qt.AlignHCenter
    }
    Action {
        id: alignRightAction
        text: qsTr('Right')
        icon.name: 'format-justify-right'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/format-justify-right.png'
        onTriggered: document.alignment = Qt.AlignRight
        checkable: true
        checked: document.alignment == Qt.AlignRight
    }
    Action {
        id: alignJustifyAction
        text: qsTr('Justify')
        icon.name: 'format-justify-fill'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/format-justify-fill.png'
        onTriggered: document.alignment = Qt.AlignJustify
        checkable: true
        checked: document.alignment == Qt.AlignJustify
    }
    Action {
        id: boldAction
        text: qsTr('Bold')
        icon.name: 'format-text-bold'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/format-text-bold.png'
        onTriggered: document.bold = !document.bold
        checkable: true
        checked: document.bold
    }
    Action {
        id: italicAction
        text: qsTr('Italic')
        icon.name: 'format-text-italic'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/format-text-italic.png'
        onTriggered: document.italic = !document.italic
        checkable: true
        checked: document.italic
    }
    Action {
        id: underlineAction
        text: qsTr('Underline')
        icon.name: 'format-text-underline'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/format-text-underline.png'
        onTriggered: document.underline = !document.underline
        checkable: true
        checked: document.underline
    }
    Action {
        id: fontFamilyAction
        text: qsTr('Font')
        icon.name: 'font'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/font.png'
        onTriggered: {
            fontDialog.font.family = document.fontFamily
            fontDialog.font.pointSize = document.fontSize
            fontDialog.open()
        }
    }
    Action {
        id: decreaseIndentAction
        text: qsTr('Decrease Indent')
        icon.name: 'format-indent-less'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/format-indent-less.png'
        onTriggered: document.indentLess()
    }
    Action {
        id: increaseIndentAction
        text: qsTr('Insert Indent')
        icon.name: 'format-indent-more'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/format-indent-more.png'
        onTriggered: document.indentMore()
    }

    FontDialog {
        id: fontDialog
        modality: application.dialogModality
        onAccepted: {
            document.fontFamily = font.family
            document.fontSize = font.pointSize
        }
    }
    ColorDialog {
        id: colorDialog
        color: 'black'
        showAlphaChannel: true
        modality: application.dialogModality
    }
    MessageDialog {
        id: errorDialog
        modality: application.dialogModality
    }

    RichText {
        id: document
        target: richTextArea
        cursorPosition: richTextArea.cursorPosition
        selectionStart: richTextArea.selectionStart
        selectionEnd: richTextArea.selectionEnd
        textColor: colorDialog.color
        onTextChanged: richTextArea.text = text
        onFontSizeChanged: {
            if (!fontSizeSpinBox.blockValue) {
                fontSizeSpinBox.blockValue = true
                fontSizeSpinBox.value = document.fontSize
                fontSizeSpinBox.blockValue = false
            }
        }
        onError: {
            errorDialog.text = message
            errorDialog.visible = true
        }
    }
}
