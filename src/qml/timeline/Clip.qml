import QtQuick 2.0

Rectangle {
    property string clipName: ''
    property string clipResource: ''
    property string mltService: ''
    property int inPoint: 0
    property int outPoint: 0
    property int clipDuration: 0
    property bool isBlank: false
    property bool isAudio: false
    property var audioLevels
    property int clipIndex

    signal clipSelected(int clipIndex)

    SystemPalette { id: activePalette }

    id: clipTop
    color: isBlank? 'transparent' : (isAudio? 'darkseagreen' : Qt.darker(activePalette.highlight))
    border.color: 'black'
    border.width: isBlank? 0 : 1
    clip: true
    state: 'normal'

    onAudioLevelsChanged: {
        if (typeof audioLevels == 'undefined') return;
        var cx = waveform.getContext('2d');
        var height = waveform.height;
        var width = waveform.width;
        cx.beginPath();
        cx.moveTo(-1, height);
        for (var i = 0; i < width; i++) {
            var level = Math.max(audioLevels[i*2], audioLevels[i*2 + 1]) / 256;
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
                target: clipTop
                border.color: 'black'
            }
        },
        State {
            name: 'selected'
            PropertyChanges {
                target: clipTop
                border.color: activePalette.highlight
                color: isBlank? 'transparent' : (isAudio? Qt.lighter('darkseagreen') : activePalette.highlight)
            }
        }
    ]

    transitions: [
        Transition {
            to: '*'
            ColorAnimation { target: clipTop; duration: 50 }
        }
    ]

    MouseArea {
        anchors.fill: parent
        enabled: !isBlank
        propagateComposedEvents: true
        onClicked: {
            parent.state = 'selected'
            parent.clipSelected(clipIndex)
            mouse.accepted = false
        }
    }
}
