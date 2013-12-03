import QtQuick 2.0
import QtQml.Models 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

Rectangle {
    id: top
    SystemPalette { id: activePalette }
    color: activePalette.window

    Column
    {
        Row {
            Column {
                id: trackHeaders
                Row {
                    id: toolbar
                    height: ruler.height
                    width: parent.width
                    spacing: 6
                    Item {
                        width: 1
                        height: 1
                    }
                    Button {
                        id: menuButton
                        implicitWidth: 28
                        implicitHeight: 24
                        iconName: 'format-justify-fill'
                        onClicked: menu.popup()
                    }
                }
                Repeater {
                    model: multitrack
                    TrackHead {
                        trackName: name
                        isMute: mute
                        isHidden: hidden
                        isVideo: !audio
                        color: (index % 2)? activePalette.alternateBase : activePalette.base
                    }
                }
            }
    
            ScrollView {
                id: scrollView
                width: top.width - trackHeaders.width
                height: top.height
                Item {
                    width: tracksContainer.width + 60
                    height: trackHeaders.height
                    Column {
                        id: tracksContainer
                        Ruler {
                            id: ruler
                            width: tracksContainer.width
                            stepSize: 40
                            index: index
                        }
                        Repeater { model: trackDelegateModel }
                    }
                    Rectangle {
                        id: playHead
                        visible: timeline.position > -1
                        color: activePalette.text
                        width: 1
                        height: scrollView.height
                        x: timeline.position
                    }
                    Canvas {
                        id: cursor
                        visible: timeline.position > -1
                        x: timeline.position - 5
                        width: 11
                        height: 5
                        onPaint:{
                            var cx = getContext('2d');
                            cx.fillStyle   = activePalette.windowText
                            cx.beginPath();
                            // Start from the top-left point.
                            cx.lineTo(11, 0);
                            cx.lineTo(5.5, 5);
                            cx.lineTo(0, 0);
                            cx.fill();
                            cx.closePath();
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: timeline.position = mouseX
                    }
                }
            }
        }
    }

    Menu {
        id: menu
        MenuItem {
            text: qsTr('New')
        }
    }

    DelegateModel {
        id: trackDelegateModel
        model: multitrack
        Track {
            model: multitrack
            rootIndex: trackDelegateModel.modelIndex(index)
            color: (index % 2)? activePalette.alternateBase : activePalette.base
        }
    }
}
