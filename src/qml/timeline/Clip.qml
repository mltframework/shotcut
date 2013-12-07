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

    SystemPalette { id: activePalette }

    color: isBlank? 'transparent' : (isAudio? 'darkseagreen' : activePalette.highlight)
    border.color: 'black'
    border.width: isBlank? 0 : 1
    clip: true
    
    Image {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: parent.border.width
        height: parent.height - parent.border.width * 2
        width: height * 16.0/9.0
        source: isAudio? '' : 'image://thumbnail/' + mltService + '/' + clipResource + '#' + outPoint
    }

    Image {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: parent.border.width
        height: parent.height - parent.border.width * 2
        width: height * 16.0/9.0
        source: isAudio? '' : 'image://thumbnail/' + mltService + '/' + clipResource + '#' + inPoint
    }

    Rectangle {
        color: 'lightgray'
        opacity: 0.7
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 2
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
            margins: 3
        }
        color: 'black'
    }
}
