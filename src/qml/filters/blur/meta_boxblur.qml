import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Blur: Box")
    mlt_service: "boxblur"
    qml: "ui_boxblur.qml"
    isHidden: true

    keyframes {
        minimumVersion: '3'
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['hori', 'vert']
        parameters: [
            Parameter {
                name: qsTr('Width')
                property: 'hori'
                isCurve: true
                minimum: 0
                maximum: 99
            },
            Parameter {
                name: qsTr('Height')
                property: 'vert'
                isCurve: true
                minimum: 0
                maximum: 99
            }
        ]
    }
}
