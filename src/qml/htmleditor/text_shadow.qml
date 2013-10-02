import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0
import QtQuick.Controls.Styles 1.0

Item {
    id: root
    width: 260
    height: 170

    signal accepted(string outline)

    GridLayout {
        id: grid_layout1
        rows: 4
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr("Horizontal")
        }

        SpinBox {
            id: horizontalSpinbox
            minimumValue: -100
            maximumValue: 100
            stepSize: 1
            Layout.fillWidth: true
            focus: true
        }

        Label{ text: qsTr('pixels') }

        Label {
            text: qsTr("Vertical")
        }

        SpinBox {
            id: verticalSpinbox
            minimumValue: -100
            maximumValue: 100
            stepSize: 1
            Layout.fillWidth: true
            focus: true
        }

        Label{ text: qsTr('pixels') }

        Label {
            text: qsTr("Softness")
        }

        SpinBox {
            id: softnessSpinbox
            minimumValue: 0
            maximumValue: 100
            stepSize: 1
            Layout.fillWidth: true
            focus: true
        }

        Label{ text: qsTr('pixels') }

        Label {
            text: qsTr("Color")
        }

        Button {
            id: colorButton
            width: 20
            height: 20
            property var color: "black"
            style: ButtonStyle {
                background: Rectangle {
                    implicitWidth: 24
                    implicitHeight: 24
                    border.width: control.activeFocus ? 2 : 1
                    border.color: "gray"
                    radius: 3
                    color: colorButton.color
                }
            }
            onClicked: colorDialog.visible = true
        }

        Item {
            Layout.fillWidth: true
        }

        Item {
            Layout.columnSpan: 3
            Layout.fillHeight: true
        }

        Item {
            width: 10
        }

        Button {
            id: okButton
            text: qsTr("OK")
            isDefault: true
            onClicked: {
                if (horizontalSpinbox.value == 0 && verticalSpinbox.value == 0 && softnessSpinbox.value == 0)
                    root.accepted('')
                else
                    root.accepted(
                                horizontalSpinbox.value + 'px '+
                                verticalSpinbox.value + 'px ' +
                                softnessSpinbox.value + 'px ' +
                                colorButton.color)
                Qt.quit()
            }
        }

        Button {
            text: qsTr("Cancel")
            onClicked: Qt.quit()
        }
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Please choose a color")
        showAlphaChannel: false
        onAccepted: {
            colorButton.color = colorDialog.color
        }
    }
}
