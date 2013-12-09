import QtQuick 2.0
import QtQml.Models 2.1
import QtQuick.Controls 1.0

Rectangle {
    id: top
    SystemPalette { id: activePalette }
    color: activePalette.window

    property int headerWidth: 120
    property int trackHeight: 50
    property real scaleFactor: 1.0

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
                }
            }
            Flickable {
                contentY: scrollView.flickableItem.contentY
                width: headerWidth
                height: trackHeaders.height
                interactive: false

                Column {
                    id: trackHeaders
                    Repeater {
                        model: multitrack
                        TrackHead {
                            trackName: model.name
                            isMute: model.mute
                            isHidden: model.hidden
                            isVideo: !model.audio
                            color: (index % 2)? activePalette.alternateBase : activePalette.base
                            width: headerWidth
                            height: model.audio? trackHeight : trackHeight * 2
                            onTrackNameChanged: {
                                if (isEditing)
                                    multitrack.setTrackName(index, trackName)
                                isEditing = false
                            }
                            onMuteClicked: {
                                multitrack.setTrackMute(index, isMute)
                            }
                            onHideClicked: {
                                multitrack.setTrackHidden(index, isHidden)
                            }
                        }
                    }
                }   
            }
            Rectangle {
                color: activePalette.window
                height: top.height - trackHeaders.height - ruler.height + 4
                width: headerWidth
                Slider {
                    id: scaleSlider
                    orientation: Qt.Horizontal
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                        leftMargin: 4
                        rightMargin: 4
                    }
                    minimumValue: 0.1
                    maximumValue: 5.0
                    value: 4.0
                    onValueChanged: {
                        if (typeof scaleFactor != 'undefined')
                            scaleFactor = (value <= 4) ? value / 4 : 1.0 + (value - 4) * 2
                        if (typeof scrollIfNeeded != 'undefined')
                            scrollIfNeeded()
                    }
                }
            }
        }

        MouseArea {
            width: top.width - headerWidth
            height: top.height
            onClicked: timeline.position = (scrollView.flickableItem.contentX + mouseX) / scaleFactor

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
                        timeScale: scaleFactor
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
                id: cursor
                visible: timeline.position > -1
                color: activePalette.text
                width: 1
                height: top.height - scrollView.__horizontalScrollBar.height
                x: timeline.position * scaleFactor - scrollView.flickableItem.contentX
                y: 0
            }
            Canvas {
                id: playhead
                visible: timeline.position > -1
                x: timeline.position * scaleFactor - scrollView.flickableItem.contentX - 5
                y: 0
                width: 11
                height: 5
                renderStrategy: Canvas.Threaded
                onAvailableChanged: {
                    var cx = getContext('2d');
                    cx.fillStyle = activePalette.windowText;
                    cx.beginPath();
                    // Start from the top-left point.
                    cx.lineTo(width, 0);
                    cx.lineTo(width / 2.0, height);
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
            height: audio? trackHeight : trackHeight * 2
            width: childrenRect.width
            color: (index % 2)? activePalette.alternateBase : activePalette.base
            z: 1
            timeScale: scaleFactor
        }
    }
    
    Connections {
        target: timeline
        onPositionChanged: scrollIfNeeded()
    }

    function scrollIfNeeded() {
        var position = timeline.position * scaleFactor;
        if (position > scrollView.width + scrollView.flickableItem.contentX - 50)
            scrollView.flickableItem.contentX = position - scrollView.width + 50;
        else if (position < 50)
            scrollView.flickableItem.contentX = 0;
        else if (position < scrollView.flickableItem.contentX + 50)
            scrollView.flickableItem.contentX = position - 50;
    }
}
