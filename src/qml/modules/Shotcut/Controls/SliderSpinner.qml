import QtQuick 2.1
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.1

RowLayout {
    spacing: -3
    property real value
    property alias minimumValue: slider.from
    property alias maximumValue: slider.to
    property real  ratio: 1.0
    property alias label: label.text
    property int decimals: 0
    property alias stepSize: spinner.stepSize
    property alias spinnerWidth: spinner.width
    property var suffix: ""
    property var prefix: ""

    SystemPalette { id: activePalette }

    onValueChanged: spinner.value = value / ratio

    Slider {
        id: slider
        stepSize: spinner.stepSize

        Layout.fillWidth: true
        activeFocusOnTab: false

        property bool isReady: false
        Component.onCompleted: {
            isReady = true
            value = parent.value
        }
        onValueChanged: if (isReady) {
            spinner.value = value / ratio
            parent.value = value
        }

        background: Rectangle {
            radius: 3
            color: activePalette.alternateBase
            border.color: 'gray'
            border.width: 1
            implicitHeight: spinner.height

            // Hide the right border.
            Rectangle {
                visible: !label.visible
                anchors {
                    top: parent.top
                    right: parent.right
                    bottom: parent.bottom
                    topMargin: 1
                    bottomMargin: 1
                }
                width: 3
                color: parent.color
            }

            // Indicate percentage full.
            Rectangle {
                anchors {
                    top: parent.top
                    left: parent.left
                    bottom: parent.bottom
                    margins: 1
                }
                radius: parent.radius
                width: parent.width
                       * (value - minimumValue) / (maximumValue - minimumValue)
                       - parent.border.width
                       - (label.visible? parent.border.width : 3)
                color: enabled? activePalette.highlight : activePalette.midlight
            }
        }

        handle: Rectangle {
        }
    }

    // Optional label between slider and spinner
    Rectangle {
        width: 4 - parent.spacing * 2
        visible: label.visible
    }
    Label {
        id: label
        visible: text.length
    }
    Rectangle {
        width: 4 - parent.spacing * 2
        visible: label.visible
    }

    SpinBox {
        id: spinner
        verticalPadding: 2
        Layout.minimumWidth: 90
        from: slider.from / ratio
        to: slider.to / ratio
        stepSize: 1 / Math.pow(10, decimals)
        onValueChanged: slider.value = value * ratio

        textFromValue: function(value, locale) {
            return "%1 %2 %3".arg(prefix).arg(Number(value).toLocaleString(locale, 'f', decimals)).arg(suffix)
        }

        background: Rectangle {
            color: activePalette.base
            border.color: 'gray'
            border.width: 1
            implicitHeight: 18
            radius: 3

            // Hide the left border.
            Rectangle {
                visible: !label.visible
                anchors {
                    top: parent.top
                    left: parent.left
                    bottom: parent.bottom
                    topMargin: 1
                    bottomMargin: 1
                }
                width: 3
                color: parent.color
            }
        }

        up.indicator: Rectangle {}
        down.indicator: Rectangle {}
    }
}
