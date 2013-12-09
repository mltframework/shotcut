import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
import QtQuick.Layouts 1.0

Rectangle {
    property string trackName: ''
    property alias isMute: muteButton.checked
    property alias isHidden: hideButton.checked
    property alias isVideo: hideButton.visible
    property bool isEditing: false
    signal muteClicked()
    signal hideClicked()

    id: trackHeadTop
    SystemPalette { id: activePalette }
    color: activePalette.window
    clip: true

    Column {
        id: trackHeadColumn
        spacing: 6
        anchors {
            top: parent.top
            left: parent.left
            margins: 4
        }

        Label {
            text: trackName
            color: activePalette.windowText
            width: trackHeadTop.width
            elide: Qt.ElideRight
            MouseArea {
                height: parent.height
                width: nameEdit.width
                onClicked: {
                    nameEdit.visible = true
                    nameEdit.selectAll()
                    isEditing = true
                }
            }
            TextField {
                id: nameEdit
                visible: false
                width: trackHeadTop.width - trackHeadColumn.anchors.margins * 2
                text: trackName
                onAccepted: {
                    trackName = text
                    visible = false
                }
                onFocusChanged: visible = focus
            }
        }
        RowLayout {
            spacing: 0
            CheckBox {
                id: muteButton
                checked: isMute
                style: CheckBoxStyle {
                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 2
                        color: control.checked? activePalette.highlight : trackHeadTop.color
                        border.color: activePalette.midlight
                        border.width: 1
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr('M')
                            color: control.checked? activePalette.highlightedText : activePalette.windowText
                        }
                    }
                }
                onClicked: muteClicked()
            }

            CheckBox {
                id: hideButton
                checked: isHidden
                style: CheckBoxStyle {
                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 2
                        color: control.checked? activePalette.highlight : trackHeadTop.color
                        border.color: activePalette.midlight
                        border.width: 1
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr('H')
                            color: control.checked? activePalette.highlightedText : activePalette.windowText
                        }
                    }
                }
                onClicked: hideClicked()
            }
        }
    }
}
