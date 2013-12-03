import QtQuick 2.0

Rectangle {
    property int stepSize: 26
    property int index: 0
    SystemPalette { id: activePalette }

    id: ruler
    height: 30
    color: activePalette.base
    Repeater {
        model: parent.width / ruler.stepSize
        Rectangle {
            anchors.bottom: ruler.bottom
            height: (index % 4)? ((index % 2) ? 3 : 7) : 14
            width: 1
            color: activePalette.windowText
            x: index * ruler.stepSize
        }
    }
    Repeater {
        model: parent.width / ruler.stepSize / 4
        Text {
            anchors.bottom: ruler.bottom
            anchors.bottomMargin: 3
            color: activePalette.windowText
            x: index * ruler.stepSize * 4 + 2
            text: timeline.timecode(index * ruler.stepSize * 4)
            font.pixelSize: 10
        }
    }
}
