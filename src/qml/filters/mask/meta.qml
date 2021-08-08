import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Mask")
    mlt_service: "frei0r.alphaspot"
    isHidden: true
    qml: "ui.qml"
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['1', '2', '3', '4']
        parameters: [
            Parameter {
                name: qsTr('Horizontal')
                property: '1'
                isCurve: true
                minimum: -1
                maximum: 1
            },
            Parameter {
                name: qsTr('Vertical')
                property: '2'
                isCurve: true
                minimum: -1
                maximum: 1
            },
            Parameter {
                name: qsTr('Width')
                property: '3'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Height')
                property: '4'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
