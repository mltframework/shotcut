/*
 * Copyright (c) 2020 Meltytech, LLC
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

import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.2
import Shotcut.Controls 1.0
import org.shotcut.qml 1.0

VuiBase {
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
    property url settingsSavePath: 'file:///' + settings.savePath

    Component.onCompleted: {
        setRectangleControl()
        filter.set('_hide', 1)
        background.color = filter.get('bgcolour')
        setTextAreaHeight()
        textArea.text = filter.get('html')
        fontSizeSpinBox.value = document.fontSize
        toolbar.expanded = filter.get('_shotcut:toolbarCollapsed') !== '1'
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
            textArea.height = filterRect.height >= profile.height? Math.max(filterRect.height, textArea.contentHeight) : filterRect.height
            break;
        case '0': // hidden
            textArea.height = filterRect.height
            break;
        default: // visible
            textArea.height = Math.max(filterRect.height, textArea.contentHeight)
        }
    }

    function updateTextSize() {
        filter.set(sizeProperty, Qt.rect(0, 0, document.size.width, document.size.height))
    }

    Flickable {
        id: flickable
        anchors.fill: parent
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
                height: textArea.height * rectangle.heightScale
            }

            TextArea {
                id: textArea
                transformOrigin: Item.TopLeft
                scale: rectangle.heightScale
                antialiasing: true
                layer.smooth: true
                smooth: true
                x: filterRect.x * scale
                y: filterRect.y * scale
                width: filterRect.width * rectangle.widthScale / scale
                height: background.height
                backgroundVisible: false
                frameVisible: false
                textFormat: Qt.RichText
                baseUrl: 'qrc:/'
                menu: Menu {
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
                }
                text: '__empty__'
                Component.onCompleted: forceActiveFocus()
                onTextChanged: {
                    if (text.indexOf('__empty__') > -1) return
                    filter.set('html', text)
                }
                onContentWidthChanged: updateTextSize()
                onContentHeightChanged: updateTextSize()
            }

            ToolBar {
                id: toolbar
                property bool expanded: filter.get('_shotcut:toolbarCollapsed') !== '1'
                property real maxWidth: 555
                x: Math.min((parent.width + parent.x - width), Math.max((-parent.x * scale), textArea.x + rectangle.handleSize))
                y: Math.min((parent.height + parent.y - height), Math.max((-parent.y * scale), (textArea.mapToItem(vui, 0, 0).y > height)? (textArea.y - height*scale) : (textArea.y + rectangle.handleSize)))
                width: expanded? (smallIcons? 380 : maxWidth) : (hiddenButton.width + (smallIcons? 0 : 8))
                Behavior on width {
                    NumberAnimation{ duration: 100 }
                }
                height: expanded? (smallIcons? (hiddenButton.height - 4) : (hiddenButton.height + 4)) : (smallIcons? hiddenButton.height - 8 : hiddenButton.height)
                anchors.margins: 0
                opacity: 0.7
                transformOrigin: Item.TopLeft
                scale: 1/zoom
                RowLayout {
                    ToolButton {
                        id: hiddenButton
                        visible: false
                    }
                    ToolButton {
                        action: menuAction
                        implicitWidth: smallIcons? 18 : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                    }
                    ToolButton {
                        action: boldAction
                        implicitWidth: smallIcons? 18 : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                    }
                    ToolButton {
                        action: italicAction
                        implicitWidth: smallIcons? 18 : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                    }
                    ToolButton {
                        action: underlineAction
                        implicitWidth: smallIcons? 18 : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                    }
                    Button { // separator
                        enabled: false
                        implicitWidth: 2
                        implicitHeight: smallIcons? 14 : (hiddenButton.implicitHeight - 8)
                        visible: toolbar.expanded
                    }
                    ToolButton {
                        action: fontFamilyAction
                        implicitWidth: smallIcons? 18 : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                    }
                    SpinBox {
                        id: fontSizeSpinBox
                        ToolTip { text: qsTr('Text size') }
                        implicitWidth: 50
                        visible: toolbar.expanded
                        value: 72
                        minimumValue: 1
                        maximumValue: 1000
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
                        tooltip: qsTr('Text color')
                        implicitWidth: toolbar.height - 4
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
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
                        implicitWidth: smallIcons? 18 : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                    }
                    ToolButton {
                        action: alignCenterAction
                        implicitWidth: smallIcons? 18 : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                    }
                    ToolButton {
                        action: alignRightAction
                        implicitWidth: smallIcons? 18 : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                    }
                    ToolButton {
                        action: alignJustifyAction
                        implicitWidth: smallIcons? 18 : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                    }
                    ToolButton {
                        action: decreaseIndentAction
                        implicitWidth: smallIcons? 18 : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                    }
                    ToolButton {
                        action: increaseIndentAction
                        implicitWidth: smallIcons? 18 : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        visible: toolbar.expanded
                    }
                    ToolButton {
                        id: expandCollapseButton
                        implicitWidth: smallIcons? 18 : hiddenButton.implicitWidth
                        implicitHeight: implicitWidth
                        tooltip: toolbar.expanded? qsTr('Collapse Toolbar') : qsTr('Expand Toolbar')
                        iconName: toolbar.expanded? 'media-seek-backward' : 'media-seek-forward'
                        iconSource: toolbar.expanded? 'qrc:///icons/oxygen/32x32/actions/media-seek-backward.png' : 'qrc:///icons/oxygen/32x32/actions/media-seek-backward.png'
                        onClicked: {
                            toolbar.expanded = !toolbar.expanded
                            filter.set('_shotcut:toolbarCollapsed', !toolbar.expanded)
                        }
                    }
                }
            }

            RectangleControl {
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
        id: menu
        MenuItem { action: fileOpenAction }
        MenuItem { action: fileSaveAsAction }
        MenuSeparator {}
        MenuItem { action: undoAction }
        MenuItem { action: redoAction }
        MenuSeparator {}
        MenuItem { action: cutAction }
        MenuItem { action: copyAction }
        MenuItem { action: pasteAction }
        MenuItem { action: pastePlainAction }
        MenuSeparator {}
        MenuItem { action: selectAllAction }
        MenuItem { action: insertTableAction }
    }

    Action {
        id: fileOpenAction
        text: qsTr('Open')
        iconName: application.OS === 'OS X'? '' : 'document-open'
        iconSource: application.OS === 'OS X'? '' : 'qrc:///icons/oxygen/32x32/actions/document-open.png'
        onTriggered: {
            fileDialog.selectExisting = true
            fileDialog.open()
        }
    }
    Action {
        id: fileSaveAsAction
        text: qsTr('Save As…')
        iconName: application.OS === 'OS X'? '' : 'document-save'
        iconSource: application.OS === 'OS X'? '' : 'qrc:///icons/oxygen/32x32/actions/document-save.png'
        onTriggered: {
            fileDialog.selectExisting = false
            fileDialog.open()
        }
    }
    Action {
        id: menuAction
        tooltip: qsTr('Menu')
        iconName: 'show-menu'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/show-menu.png'
        onTriggered: menu.popup()
    }
    Action {
        id: undoAction
        text: qsTr('Undo')
        shortcut: 'ctrl+z'
        iconName: application.OS === 'OS X'? '' : 'edit-undo'
        iconSource: application.OS === 'OS X'? '' : 'qrc:///icons/oxygen/32x32/actions/edit-undo.png'
        onTriggered: textArea.undo()
    }
    Action {
        id: redoAction
        text: qsTr('Redo')
        shortcut: application.OS === 'Windows'? 'ctrl+y' : 'ctrl+shift+z'
        iconName: application.OS === 'OS X'? '' : 'edit-redo'
        iconSource: application.OS === 'OS X'? '' : 'qrc:///icons/oxygen/32x32/actions/edit-redo.png'
        onTriggered: textArea.redo()
    }
    Action {
        id: cutAction
        text: qsTr('Cut')
        shortcut: 'ctrl+x'
        iconName: application.OS === 'OS X'? '' : 'edit-cut'
        iconSource: application.OS === 'OS X'? '' : 'qrc:///icons/oxygen/32x32/actions/edit-cut.png'
        onTriggered: textArea.cut()
    }
    Action {
        id: copyAction
        text: qsTr('Copy')
        shortcut: 'ctrl+c'
        iconName: application.OS === 'OS X'? '' : 'edit-copy'
        iconSource: application.OS === 'OS X'? '' : 'qrc:///icons/oxygen/32x32/actions/edit-copy.png'
        onTriggered: textArea.copy()
    }
    Action {
        id: pasteAction
        text: qsTr('Paste')
        shortcut: 'ctrl+v'
        iconName: application.OS === 'OS X'? '' : 'edit-paste'
        iconSource: application.OS === 'OS X'? '' : 'qrc:///icons/oxygen/32x32/actions/edit-paste.png'
        onTriggered: textArea.paste()
    }
    Action {
        id: pastePlainAction
        text: qsTr('Paste Text Only')
        onTriggered: document.pastePlain()
    }
    Action {
        id: deleteAction
        text: qsTr('Delete')
        shortcut: 'del'
        iconName: application.OS === 'OS X'? '' : 'edit-delete'
        iconSource: application.OS === 'OS X'? '' : 'qrc:///icons/oxygen/32x32/actions/edit-delete.png'
        onTriggered: textArea.remove(textArea.selectionStart, textArea.selectionEnd)
    }
    Action {
        id: clearAction
        text: qsTr('Clear')
        iconName: application.OS === 'OS X'? '' : 'edit-clear'
        iconSource: application.OS === 'OS X'? '' : 'qrc:///icons/oxygen/32x32/actions/edit-clear.png'
        onTriggered: {
            textArea.selectAll()
            textArea.remove(textArea.selectionStart, textArea.selectionEnd)
        }
    }
    Action {
        id: selectAllAction
        text: qsTr('Select All')
        shortcut: 'ctrl+a'
        onTriggered: textArea.selectAll()
    }

    Action {
        id: alignLeftAction
        text: qsTr('Left')
        iconName: 'format-justify-left'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/format-justify-left.png'
        onTriggered: document.alignment = Qt.AlignLeft
        checkable: true
        checked: document.alignment == Qt.AlignLeft
    }
    Action {
        id: alignCenterAction
        text: qsTr('Center')
        iconName: 'format-justify-center'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/format-justify-center.png'
        onTriggered: document.alignment = Qt.AlignHCenter
        checkable: true
        checked: document.alignment == Qt.AlignHCenter
    }
    Action {
        id: alignRightAction
        text: qsTr('Right')
        iconName: 'format-justify-right'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/format-justify-right.png'
        onTriggered: document.alignment = Qt.AlignRight
        checkable: true
        checked: document.alignment == Qt.AlignRight
    }
    Action {
        id: alignJustifyAction
        text: qsTr('Justify')
        iconName: 'format-justify-fill'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/format-justify-fill.png'
        onTriggered: document.alignment = Qt.AlignJustify
        checkable: true
        checked: document.alignment == Qt.AlignJustify
    }
    Action {
        id: boldAction
        text: qsTr('Bold')
        iconName: 'format-text-bold'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/format-text-bold.png'
        onTriggered: document.bold = !document.bold
        checkable: true
        checked: document.bold
    }
    Action {
        id: italicAction
        text: qsTr('Italic')
        iconName: 'format-text-italic'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/format-text-italic.png'
        onTriggered: document.italic = !document.italic
        checkable: true
        checked: document.italic
    }
    Action {
        id: underlineAction
        text: qsTr('Underline')
        iconName: 'format-text-underline'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/format-text-underline.png'
        onTriggered: document.underline = !document.underline
        checkable: true
        checked: document.underline
    }
    Action {
        id: fontFamilyAction
        text: qsTr('Font')
        iconName: 'font'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/font.png'
        onTriggered: {
            fontDialog.font.family = document.fontFamily
            fontDialog.font.pointSize = document.fontSize
            fontDialog.open()
        }
    }
    Action {
        id: insertTableAction
        text: qsTr('Insert Table')
        iconName: application.OS === 'OS X'? '' : 'view-grid'
        iconSource: application.OS === 'OS X'? '' : 'qrc:///icons/oxygen/32x32/actions/view-grid.png'
        onTriggered: tableDialog.open()
    }
    Action {
        id: decreaseIndentAction
        text: qsTr('Decrease Indent')
        iconName: 'format-indent-less'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/format-indent-less.png'
        onTriggered: document.indentLess()
    }
    Action {
        id: increaseIndentAction
        text: qsTr('Insert Indent')
        iconName: 'format-indent-more'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/format-indent-more.png'
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
        modality: Qt.ApplicationModal
        onAccepted: {
            document.fontFamily = font.family
            document.fontSize = font.pointSize
        }
    }
    ColorDialog {
        id: colorDialog
        color: 'black'
        showAlphaChannel: true
        modality: Qt.ApplicationModal
    }
    MessageDialog {
        id: errorDialog
        modality: Qt.ApplicationModal
    }
    Dialog {
        id: tableDialog
        title: qsTr('Insert Table')
        standardButtons: StandardButton.Ok | StandardButton.Cancel
        modality: Qt.ApplicationModal
        GridLayout {
            rows: 4
            columns: 2
            anchors.fill: parent
            anchors.margins: 8

            Label {
                text: qsTr('Rows')
                Layout.alignment: Qt.AlignRight
            }
            SpinBox {
                id: rowsSpinner
                value: 1
                minimumValue: 1
                maximumValue: 100
                stepSize: 1
                focus: true
            }
            Label {
                text: qsTr('Columns')
                Layout.alignment: Qt.AlignRight
            }
            SpinBox {
                id: columnsSpinner
                value: 2
                minimumValue: 1
                maximumValue: 100
                stepSize: 1
            }
            Label {
                text: qsTr('Border')
                Layout.alignment: Qt.AlignRight
            }
            SpinBox {
                id: borderSpinner
                value: 0
                minimumValue: 0
                maximumValue: 100
                stepSize: 1
                suffix: ' px'
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
