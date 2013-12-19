import QtQuick 2.0

Rectangle {
    id: clipRoot
    property string clipName: ''
    property string clipResource: ''
    property string mltService: ''
    property int inPoint: 0
    property int outPoint: 0
    property int clipDuration: 0
    property bool isBlank: false
    property bool isAudio: false
    property var audioLevels
    property int trackIndex
    property int originalX: x

    signal selected(var clip)
    signal moved(var clip)
    signal dragged(var clip, var mouse)
    signal dropped(var clip)
    signal draggedToTrack(var clip, int direction)
    signal trimmingIn(var clip, real delta)
    signal trimmedIn(var clip)
    signal trimmingOut(var clip, real delta)
    signal trimmedOut(var clip)

    SystemPalette { id: activePalette }

    color: getColor()
    border.color: 'black'
    border.width: isBlank? 0 : 1
    clip: true
    state: 'normal'
    Drag.active: mouseArea.drag.active
    Drag.proposedAction: Qt.MoveAction
//    z: Drag.active? 1 : 0

    function getColor() {
        return isBlank? 'transparent' : (isAudio? 'darkseagreen' : Qt.darker(activePalette.highlight))
    }

    function reparent(track) {
        parent = track
        isAudio = track.isAudio
        height = track.height
        generateWaveform()
    }

    function generateWaveform() {
        if (typeof audioLevels == 'undefined') return;
        var cx = waveform.getContext('2d');
        var height = waveform.height;
        var width = waveform.width;
        var color = getColor();
        cx.beginPath();
        cx.moveTo(-1, height);
        for (var i = 0; i < width; i++) {
            var j = Math.round(i / timeScale);
            var level = Math.max(audioLevels[j*2], audioLevels[j*2 + 1]) / 256;
            cx.lineTo(i, height - level * height);
        }
        cx.lineTo(width, height);
        cx.closePath();
        cx.fillStyle = Qt.lighter(color);
        cx.fill();
        cx.strokeStyle = Qt.darker(color);
        cx.stroke();
        waveform.requestPaint();
    }

    onAudioLevelsChanged: generateWaveform()

    Image {
        id: inThumbnail
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: parent.border.width
        height: (parent.height - parent.border.width * 2) / 2
        width: height * 16.0/9.0
        source: (isAudio || isBlank)? '' : 'image://thumbnail/' + mltService + '/' + clipResource + '#' + outPoint
    }

    Image {
        id: outThumbnail
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: parent.border.width
        height: (parent.height - parent.border.width * 2) / 2
        width: height * 16.0/9.0
        source: (isAudio || isBlank)? '' : 'image://thumbnail/' + mltService + '/' + clipResource + '#' + inPoint
    }

    Canvas {
        id: waveform
        width: parent.width - parent.border.width * 2
        height: isAudio? parent.height : parent.height / 2
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: parent.border.width
        opacity: 0.7
    }

    Rectangle {
        width: parent.width - parent.border.width * 2
        height: 1
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: parent.border.width
        anchors.bottomMargin: waveform.height * 0.9
        color: Qt.darker(parent.color)
        opacity: 0.7
    }

    Rectangle {
        color: 'lightgray'
        visible: !isBlank
        opacity: 0.7
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: parent.border.width
        anchors.leftMargin: parent.border.width + (isAudio? 0 : inThumbnail.width)
        width: label.width + 2
        height: label.height
    }

    Text {
        id: label
        text: name
        visible: !isBlank
        font.pointSize: 8
        anchors {
            top: parent.top
            left: parent.left
            topMargin: parent.border.width + 1
            leftMargin: parent.border.width + (isAudio? 0 : inThumbnail.width) + 1
        }
        color: 'black'
    }

    states: [
        State {
            name: 'normal'
            PropertyChanges {
                target: clipRoot
                border.color: 'black'
                z: 0
            }
        },
        State {
            name: 'selected'
            PropertyChanges {
                target: clipRoot
                border.color: activePalette.highlight
                color: isBlank? 'transparent' : (isAudio? Qt.lighter('darkseagreen') : activePalette.highlight)
                z: 1
            }
        }
    ]

    transitions: [
        Transition {
            to: '*'
            ColorAnimation { target: clipRoot; duration: 50 }
        }
    ]

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: !isBlank
        propagateComposedEvents: true
        drag.target: parent
        drag.axis: Drag.XAxis
        cursorShape: drag.active? Qt.DragMoveCursor : Qt.ArrowCursor
        onClicked: {
            mouse.accepted = false
        }
        property int startX
        onPressed: {
            originalX = parent.x
            startX = parent.x
            parent.state = 'selected'
            parent.selected(clipRoot)
        }
        onPositionChanged: {
            if (mouse.y < 0)
                parent.draggedToTrack(clipRoot, -1)
            else if (mouse.y > height)
                parent.draggedToTrack(clipRoot, 1)
            parent.dragged(clipRoot, mouse)
        }
        onReleased: {
            parent.y = 0
            var delta = parent.x - startX
            if (Math.abs(delta) > 0) {
                parent.moved(clipRoot)
                originalX = parent.x
            } else {
                parent.dropped(clipRoot)
            }
        }
    }

    Rectangle {
        id: trimIn
        enabled: !isBlank
        anchors.left: parent.left
        anchors.leftMargin: 0
        height: parent.height
        width: 5
        opacity: 0
        Drag.active: trimInMouseArea.drag.active
        Drag.proposedAction: Qt.MoveAction

        MouseArea {
            id: trimInMouseArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            property int startX

            onPressed: {
                startX = parent.x
                parent.anchors.left = undefined
            }
            onReleased: {
                parent.anchors.left = clipRoot.left
                clipRoot.trimmedIn(clipRoot)
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((parent.x - startX) / timeScale)
                    if (Math.abs(delta) > 0) {
                        clipRoot.trimmingIn(clipRoot, delta)
                    }
                }
            }
        }
    }
    Rectangle {
        id: trimOut
        enabled: !isBlank
        anchors.right: parent.right
        anchors.rightMargin: 0
        height: parent.height
        width: 5
        color: 'red'
        opacity: 0
        Drag.active: trimOutMouseArea.drag.active
        Drag.proposedAction: Qt.MoveAction

        MouseArea {
            id: trimOutMouseArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            property int duration

            onPressed: {
                duration = clipDuration
                parent.anchors.right = undefined
            }
            onReleased: {
                parent.anchors.right = clipRoot.right
                clipRoot.trimmedOut(clipRoot)
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var newDuration = Math.round((parent.x + parent.width) / timeScale)
                    var delta = duration - newDuration 
                    if (Math.abs(delta) > 0) {
                        clipRoot.trimmingOut(clipRoot, delta)
                        duration = newDuration
                    }
                }
            }
        }
    }
}
