import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Blur")
    mlt_service: "boxblur"
    qml: "ui_boxblur.qml"
    gpuAlt: "movit.blur"
    keyframes {
        minimumVersion: '3'
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['hori', 'vert']
        parameters: [
            Parameter {
                name: qsTr('Width')
                property: 'hori'
                isSimple: true
                isCurve: true
                minimum: 1
                maximum: 99
            },
            Parameter {
                name: qsTr('Height')
                property: 'vert'
                isSimple: true
                isCurve: true
                minimum: 1
                maximum: 99
            }
        ]
    }
}
