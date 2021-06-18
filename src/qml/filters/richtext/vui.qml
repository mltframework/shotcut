/*
 * Copyright (c) 2020-2021 Meltytech, LLC
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
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.2
import Shotcut.Controls 1.0 as Shotcut
import org.shotcut.qml 1.0

Shotcut.VuiBase {
    id: vui
    property string rectProperty: 'geometry'
    property string halignProperty: 'valign'
    property string valignProperty: 'halign'
    property string useFontSizeProperty: 'shotcut:usePointSize'
    property real zoom: (video.zoom > 0)? video.zoom : 1.0
    property rect filterRect: Qt.rect(-1, -1, -1, -1)
    property bool blockUpdate: false
    property string startValue: '_shotcut:startValue'
    property string middleValue: '_shotcut:middleValue'
    property string endValue:  '_shotcut:endValue'
    property string sizeProperty: '_shotcut:size'
    property bool smallIcons: settings.smallIcons || toolbar.maxWidth >= videoItem.width
    property int smallIconSize: 22
    property url settingsSavePath: 'file:///' + settings.savePath

    Component.onCompleted: {
        setRectangleControl()
        filter.set('_hide', 1)
        background.color = filter.get('bgcolour')
        setTextAreaHeight()
        textArea.text = filter.get('html')
        fontSizeSpinBox.value = document.fontSize
        toolbar.expanded = filter.get('_shotcut:toolbarCollapsed') !== '1'
        application.showStatusMessage(qsTr('Click in the rectangle to edit the text'))
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setRectangleControl() {
        if (blockUpdate) return
        var position = getPosition()
        var newValue = filter.getRect(rectProperty, position)
        if (filterRect !== newValue) {
            filterRect = newValue
            rectangle.setHandles(filterRect)
        }
        rectangle.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
    }

    function updateFilter(position) {
        blockUpdate = true
        var rect = rectangle.rectangle
        filterRect.x = Math.round(rect.x / rectangle.widthScale)
        filterRect.y = Math.round(rect.y / rectangle.heightScale)
        filterRect.width = Math.round(rect.width / rectangle.widthScale)
        filterRect.height = Math.round(rect.height / rectangle.heightScale)

        if (position !== null) {
            filter.blockSignals = true
            if (position <= 0 && filter.animateIn > 0)
                filter.set(startValue, filterRect)
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                filter.set(endValue, filterRect)
            else
                filter.set(middleValue, filterRect)
            filter.blockSignals = false
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(rectProperty)
            if (filter.animateIn > 0) {
                filter.set(rectProperty, filter.getRect(startValue), 1.0, 0)
                filter.set(rectProperty, filter.getRect(middleValue), 1.0, filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set(rectProperty, filter.getRect(middleValue), 1.0, filter.duration - filter.animateOut)
                filter.set(rectProperty, filter.getRect(endValue), 1.0, filter.duration - 1)
            }
        } else if (filter.keyframeCount(rectProperty) <= 0) {
            filter.resetProperty(rectProperty)
            filter.set(rectProperty, filter.getRect(middleValue))
        } else if (position !== null) {
            filter.set(rectProperty, filterRect, 1.0, position)
        }
        blockUpdate = false
        filter.set(sizeProperty, Qt.rect(0, 0, document.size.width, document.size.height))
    }

    function setTextAreaHeight() {
        switch (filter.get('overflow-y')) {
        case '':
            scrollView.height = filterRect.height >= profile.height? Math.max(filterRect.height, textArea.contentHeight) : filterRect.height
            break;
        case '0': // hidden
            scrollView.height = filterRect.height
            break;
        default: // visible
            scrollView.height = Math.max(filterRect.height, textArea.contentHeight)
        }
    }

    function updateTextSize() {
        filter.set(sizeProperty, Qt.rect(0, 0, document.size.width, document.size.height))
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        flickableDirection: Flickable.VerticalFlick
        interactive: false
        clip: true
        contentWidth: video.rect.width * zoom
        contentHeight: video.rect.height * zoom
        contentX: video.offset.x
        contentY: video.offset.y

        Item {
            id: videoItem
            x: video.rect.x
            y: video.rect.y
            width: video.rect.width
            height: video.rect.height
            scale: zoom

            Rectangle {
                id: background
                x: rectangle.rectangle.x
                y: rectangle.rectangle.y
                width: rectangle.rectangle.width
                height: scrollView.height * rectangle.heightScale
            }

            ScrollView {
                id: scrollView
                transformOrigin: Item.TopLeft
                scale: rectangle.heightScale
                x: filterRect.x * scale
                y: filterRect.y * scale
                width: filterRect.width * rectangle.widthScale / scale
                padding: 0
                ScrollBar.vertical.width: 16 / scale
                ScrollBar.vertical.x: width - ScrollBar.vertical.width - rectangle.handleSize

                TextArea {
                    id: textArea
                    padding: 0
                    textFormat: Qt.RichText
                    selectByMouse: true
                    persistentSelection: true
                    wrapMode: TextArea.Wrap
                    cursorDelegate: Rectangle {
                        id: cursor
                        visible: textArea.cursorVisible
                        width: 2.5/scale
                        color: 'white'
                        SequentialAnimation {
                            running: cursor.visible
                            loops: Animation.Infinite
                            NumberAnimation {
                                target: cursor
                                property: 'opacity'
                                from: 0
                                to: 1
                                duration: 100
                            }
                            PauseAnimation { duration: 400 }
                            NumberAnimation {
                                target: cursor
                                property: 'opacity'
                                from: 1
                                to: 0
                                duration: 100
                            }
                            PauseAnimation { duration: 400 }
                        }
                    }
                    baseUrl: 'qrc:/'
                    MouseArea {
                        acceptedButtons: Qt.RightButton
                        anchors.fill: parent
                        onClicked: contextMenu.popup()
                    }
                    text: '__empty__'
                    Component.onCompleted: forceActiveFocus()
                    onTextChanged: {
                        if (text.indexOf('__empty__') > -1) return
                        filter.set('html', text)
                    }
                    onContentWidthChanged: updateTextSize()
                    onContentHeightChanged: updateTextSize()
                    Keys.onPressed: {
                        if (event.key === Qt.Key_V && (event.modifiers & Qt.ShiftModifier) &&
                            (event.modifiers & Qt.ControlModifier || event.modifiers & Qt.MetaModifier)) {
                            event.accepted = true
                            document.pastePlain()
                        }
                    }
                }
            }

            ToolBar {
                id: toolbar
                property bool expanded: filter.get('_shotcut:toolbarCollapsed') !== '1'
                property real maxWidth: 500
                x: Math.min((parent.width + parent.x - width), Math.max((-parent.x * scale), scrollView.x + rectangle.handleSize))
                y: Math.min((parent.height + parent.y - height), Math.max((-parent.y * scale), (scrollView.mapToItem(vui, 0, 0).y > height)? (scrollView.y - height*scale) : (scrollView.y + rectangle.handleSize)))
                Behavior on width {
                    NumberAnimation{ duration: 100 }
                }
                anchors.margins: 0
                opacity: 0.7
                transformOrigin: Item.TopLeft
                scale: 1/zoom

                RowLayout {
                    ToolButton {
                        id: hiddenButton
                        visible: false
                        action: Action {
                            icon.source: 'qrc:///icons/oxygen/32x32/actions/show-menu.png'
                        }
                    }
                    ToolButton {
                        Shotcut.HoverTip { text: qsTr('Menu') }
                        implicitWidth: smallIcons? smallIconSize : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                        focusPolicy: Qt.NoFocus
                        action: Action {
                            icon.name: 'show-menu'
                            icon.source: 'qrc:///icons/oxygen/32x32/actions/show-menu.png'
                            onTriggered: menu.popup()
                        }
                    }
                    ToolButton {
                        action: boldAction
                        implicitWidth: smallIcons? smallIconSize : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                        focusPolicy: Qt.NoFocus
                        Shotcut.HoverTip { text: parent.action.text }
                    }
                    ToolButton {
                        action: italicAction
                        implicitWidth: smallIcons? smallIconSize : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                        focusPolicy: Qt.NoFocus
                        Shotcut.HoverTip { text: parent.action.text }
                    }
                    ToolButton {
                        action: underlineAction
                        implicitWidth: smallIcons? smallIconSize : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                        focusPolicy: Qt.NoFocus
                        Shotcut.HoverTip { text: parent.action.text }
                    }
                    Button { // separator
                        enabled: false
                        implicitWidth: 2
                        implicitHeight: smallIcons? 14 : (hiddenButton.implicitHeight - 8)
                        visible: toolbar.expanded
                    }
                    ToolButton {
                        Shotcut.HoverTip { text: qsTr('Font') }
                        implicitWidth: smallIcons? smallIconSize : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                        focusPolicy: Qt.NoFocus
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
                        Shotcut.HoverTip { text: qsTr('Text size') }
                        implicitWidth: 60
                        visible: toolbar.expanded
                        value: 72
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
                    ToolButton {
                        id: colorButton
                        Shotcut.HoverTip { text: qsTr('Text color') }
                        implicitWidth: toolbar.height - 4
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                        focusPolicy: Qt.NoFocus
                        property var color : document.textColor
                        Rectangle {
                            id: colorRect
                            anchors.fill: parent
                            anchors.margins: 4
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
                        implicitHeight: smallIcons? 14 : (hiddenButton.implicitHeight - 8)
                        visible: toolbar.expanded
                    }
                    ToolButton {
                        action: alignLeftAction
                        implicitWidth: smallIcons? smallIconSize : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                        focusPolicy: Qt.NoFocus
                        Shotcut.HoverTip { text: parent.action.text }
                    }
                    ToolButton {
                        action: alignCenterAction
                        implicitWidth: smallIcons? smallIconSize : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                        focusPolicy: Qt.NoFocus
                        Shotcut.HoverTip { text: parent.action.text }
                    }
                    ToolButton {
                        action: alignRightAction
                        implicitWidth: smallIcons? smallIconSize : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                        focusPolicy: Qt.NoFocus
                        Shotcut.HoverTip { text: parent.action.text }
                    }
                    ToolButton {
                        action: alignJustifyAction
                        implicitWidth: smallIcons? smallIconSize : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                        focusPolicy: Qt.NoFocus
                        Shotcut.HoverTip { text: parent.action.text }
                    }
                    ToolButton {
                        action: decreaseIndentAction
                        implicitWidth: smallIcons? smallIconSize : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                        focusPolicy: Qt.NoFocus
                        Shotcut.HoverTip { text: parent.action.text }
                    }
                    ToolButton {
                        action: increaseIndentAction
                        implicitWidth: smallIcons? smallIconSize : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                        focusPolicy: Qt.NoFocus
                        Shotcut.HoverTip { text: parent.action.text }
                    }
                    ToolButton {
                        id: expandCollapseButton
                        implicitWidth: smallIcons? smallIconSize : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        focusPolicy: Qt.NoFocus
                        Shotcut.HoverTip { text: toolbar.expanded? qsTr('Collapse Toolbar') : qsTr('Expand Toolbar') }
                        action: Action {
                            icon.name: toolbar.expanded? 'media-seek-backward' : 'media-seek-forward'
                            icon.source: toolbar.expanded? 'qrc:///icons/oxygen/32x32/actions/media-seek-backward.png' : 'qrc:///icons/oxygen/32x32/actions/media-seek-backward.png'
                            onTriggered: {
                                toolbar.expanded = !toolbar.expanded
                                filter.set('_shotcut:toolbarCollapsed', !toolbar.expanded)
                            }
                        }
                    }
                }
            }

            Shotcut.RectangleControl {
                id: rectangle
                widthScale: video.rect.width / profile.width
                heightScale: video.rect.height / profile.height
                handleSize: Math.max(Math.round(8 / zoom), 4)
                borderSize: Math.max(Math.round(1.33 / zoom), 1)
                onWidthScaleChanged: setHandles(filterRect)
                onHeightScaleChanged: setHandles(filterRect)
                onRectChanged: updateFilter(getPosition())
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
        MenuItem { action: pastePlainAction }
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
            title: qsTr('File')
            MenuItem { action: fileOpenAction }
            MenuItem { action: fileSaveAsAction }
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
            MenuItem { action: pastePlainAction }
        }
        MenuItem { action: selectAllAction }
        MenuItem { action: insertTableAction }
        MenuItem {
            text: qsTr('Cancel')
            onTriggered: menu.dismiss()
        }
    }

    Action {
        id: fileOpenAction
        text: qsTr('Open...')
        onTriggered: {
            fileDialog.selectExisting = true
            fileDialog.open()
        }
    }
    Action {
        id: fileSaveAsAction
        text: qsTr('Save As…')
        onTriggered: {
            fileDialog.selectExisting = false
            fileDialog.open()
        }
    }
    Action {
        id: menuAction
        icon.name: 'show-menu'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/show-menu.png'
        onTriggered: menu.popup()
    }
    Action {
        id: undoAction
        text: qsTr('Undo') + (application.OS === 'OS X'? '    ⌘Z' : ' (Ctrl+Z)')
        onTriggered: textArea.undo()
    }
    Action {
        id: redoAction
        text: qsTr('Redo') + (application.OS === 'Windows'? ' (Ctrl+Y)' : application.OS === 'OS X'? '    ⇧⌘Z' : ' (Ctrl+Shift+Z)')
        onTriggered: textArea.redo()
    }
    Action {
        id: cutAction
        text: qsTr('Cut') + (application.OS === 'OS X'? '    ⌘X' : ' (Ctrl+X)')
        onTriggered: textArea.cut()
    }
    Action {
        id: copyAction
        text: qsTr('Copy') + (application.OS === 'OS X'? '    ⌘C' : ' (Ctrl+C)')
        onTriggered: textArea.copy()
    }
    Action {
        id: pasteAction
        text: qsTr('Paste') + (application.OS === 'OS X'? '    ⌘V' : ' (Ctrl+V)')
        onTriggered: textArea.paste()
    }
    Action {
        id: pastePlainAction
        text: qsTr('Paste Text Only') + (application.OS === 'OS X'? '    ⇧⌘V' : ' (Ctrl+Shift+V)')
        onTriggered: document.pastePlain()
    }
    Action {
        id: deleteAction
        text: qsTr('Delete')
        onTriggered: textArea.remove(textArea.selectionStart, textArea.selectionEnd)
    }
    Action {
        id: clearAction
        text: qsTr('Clear')
        onTriggered: {
            textArea.selectAll()
            textArea.remove(textArea.selectionStart, textArea.selectionEnd)
        }
    }
    Action {
        id: selectAllAction
        text: qsTr('Select All') + (application.OS === 'OS X'? '    ⌘A' : ' (Ctrl+A)')
        onTriggered: textArea.selectAll()
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
        id: insertTableAction
        text: qsTr('Insert Table')
        onTriggered: tableDialog.open()
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

    FileDialog {
        id: fileDialog
        modality: application.dialogModality
        folder: settingsSavePath
        nameFilters: ["HTML files (*.html *.htm)", "Text files (*.txt)", "All files (*)"]
        onAccepted: {
            if (fileDialog.selectExisting)
                document.fileUrl = fileUrl
            else
                document.saveAs(fileUrl, selectedNameFilter)
        }
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
    Dialog {
        id: tableDialog
        title: qsTr('Insert Table')
        standardButtons: StandardButton.Ok | StandardButton.Cancel
        modality: application.dialogModality
        GridLayout {
            rows: 4
            columns: 2
            anchors.fill: parent
            anchors.margins: 8

            Label {
                text: qsTr('Rows')
                Layout.alignment: Qt.AlignRight
            }
            Shotcut.DoubleSpinBox {
                id: rowsSpinner
                implicitWidth: 75
                value: 1
                from: 1
                to: 100
                stepSize: 1
                decimals: 0
                focus: true
            }
            Label {
                text: qsTr('Columns')
                Layout.alignment: Qt.AlignRight
            }
            Shotcut.DoubleSpinBox {
                id: columnsSpinner
                implicitWidth: 75
                value: 2
                from: 1
                to: 100
                stepSize: 1
                decimals: 0
            }
            Label {
                text: qsTr('Border')
                Layout.alignment: Qt.AlignRight
            }
            Shotcut.DoubleSpinBox {
                id: borderSpinner
                implicitWidth: 75
                value: 0
                from: 0
                to: 100
                stepSize: 1
                decimals: 0
            }
            Item { Layout.fillHeight: true; height: columnsSpinner.height }
        }
        onAccepted: {
            document.insertTable(rowsSpinner.value, columnsSpinner.value, borderSpinner.value)
        }
    }

    RichText {
        id: document
        target: textArea
        cursorPosition: textArea.cursorPosition
        selectionStart: textArea.selectionStart
        selectionEnd: textArea.selectionEnd
        textColor: colorDialog.color
        onTextChanged: textArea.text = text
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

    Connections {
        target: filter
        onChanged: {
            setRectangleControl()
            videoItem.enabled = filter.get('disable') !== '1'
            background.color = filter.get('bgcolour')
            setTextAreaHeight()
        }
    }

    Connections {
        target: producer
        onPositionChanged: setRectangleControl()
    }
}
