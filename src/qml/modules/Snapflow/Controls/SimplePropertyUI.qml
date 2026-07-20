import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root

    property var properties: []

    Layout.columnSpan: 4
    Layout.fillWidth: true
    spacing: 4

    Label {
        text: qsTr("Custom Properties")
        font.bold: true
    }

    Repeater {
        model: root.properties

        RowLayout {
            width: parent.width
            height: propField.height

            Label {
                Layout.preferredWidth: editButton.width
                Layout.minimumWidth: implicitWidth
                text: modelData

                anchors {
                    verticalCenter: propField.verticalCenter
                }
            }

            TextField {
                id: propField

                Layout.fillWidth: true
                selectByMouse: true
                text: filter.get(modelData)
                onTextChanged: filter.set(modelData, text)
            }
        }
    }
}
