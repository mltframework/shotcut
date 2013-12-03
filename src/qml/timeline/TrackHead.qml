import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

Rectangle {
    property string trackName: ''
    property alias isMute: muteButton.checked
    property alias isHidden: hideButton.checked
    property alias isVideo: hideButton.visible

    SystemPalette { id: activePalette }

    width: 100
    height: 60
    color: activePalette.window
//    border.width: 1
//    border.color: 'black'

    Column {
        spacing: 6
        anchors {
            top: parent.top
            left: parent.left
            margins: 4
        }

        Text {
            text: trackName
            color: activePalette.windowText
        }
        RowLayout {
            spacing: 6
            Button {
                id: muteButton
                checkable: true
                iconName: 'dialog-cancel'
                iconSource: 'qrc:/icons/oxygen/16x16/actions/dialog-cancel.png'
                implicitWidth: 20
                implicitHeight: 20
                tooltip: checked? 'Muted' : 'Mute'
            }
            Button {
                id: hideButton
                checkable: true
                checked: isBlind
                iconName: 'list-remove'
                iconSource: 'qrc:/icons/oxygen/16x16/actions/list-remove.png'
                implicitWidth: 20
                implicitHeight: 20
                tooltip: checked? 'Hidden' : 'Hide'
            }
        }
    }
}
