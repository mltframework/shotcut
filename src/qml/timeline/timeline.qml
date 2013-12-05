import QtQuick 2.0
import QtQml.Models 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

Rectangle {
    id: top
    SystemPalette { id: activePalette }
    color: activePalette.window

    property int headerWidth: 120
    property int trackHeight: 60

    Row {
        Column {
            z: 1
            Rectangle {
                id: toolbar
                height: ruler.height
                width: headerWidth
                z: 1
                color: activePalette.window
                Row {
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
                    Slider {
                        id: scaleSlider
                        orientation: Qt.Horizontal
                        width: headerWidth - menuButton.width - 20
                        minimumValue: 0.01
                        maximumValue: 4.0
                        value: 1.0
                        onValueChanged: scrollIfNeeded()
                    }
                }
            }
            Flickable {
                contentY: scrollView.flickableItem.contentY
                width: headerWidth
                height: top.height
                interactive: false

                Column {
                    id: trackHeaders
                    Repeater {
                        model: multitrack
                        TrackHead {
                            trackName: name
                            isMute: mute
                            isHidden: hidden
                            isVideo: !audio
                            color: (index % 2)? activePalette.alternateBase : activePalette.base
                            width: headerWidth
                            height: trackHeight
                        }
                    }
                }   
            }
        }

        MouseArea {
            width: top.width - headerWidth
            height: top.height
            onClicked: timeline.position = (scrollView.flickableItem.contentX + mouseX) / scaleSlider.value

            Column {
                Flickable {
                    contentX: scrollView.flickableItem.contentX
                    width: top.width - headerWidth
                    height: ruler.height
                    interactive: false
                    Ruler {
                        id: ruler
                        width: tracksContainer.width
                        index: index
                        timeScale: scaleSlider.value
                    }
                }
                ScrollView {
                    id: scrollView
                    width: top.width - headerWidth
                    height: top.height - ruler.height
        
                    Item {
                        width: tracksContainer.width + headerWidth
                        height: trackHeaders.height + 30 // 30 is padding
                        Column {
                            id: tracksContainer
                            Repeater { model: trackDelegateModel }
                        }
                    }
                }
            }
            Rectangle {
                id: playHead
                visible: timeline.position > -1
                color: activePalette.text
                width: 1
                height: top.height - scrollView.__horizontalScrollBar.height
                x: timeline.position * scaleSlider.value - scrollView.flickableItem.contentX
                y: 0
            }
            Canvas {
                id: cursor
                visible: timeline.position > -1
                x: timeline.position * scaleSlider.value - scrollView.flickableItem.contentX - 5
                y: 0
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
            height: trackHeight
            width: childrenRect.width
            color: (index % 2)? activePalette.alternateBase : activePalette.base
            z: 1
            timeScale: scaleSlider.value
        }
    }
    
    Connections {
        target: timeline
        onPositionChanged: scrollIfNeeded()
    }

    function scrollIfNeeded() {
        var position = timeline.position * scaleSlider.value;
        if (position > scrollView.width + scrollView.flickableItem.contentX - 50)
            scrollView.flickableItem.contentX = position - scrollView.width + 50;
        else if (position < 50)
            scrollView.flickableItem.contentX = 0;
        else if (position < scrollView.flickableItem.contentX + 50)
            scrollView.flickableItem.contentX = position - 50;
    }
}
