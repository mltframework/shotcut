import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Blur: Box")
    mlt_service: "box_blur"
    qml: "ui_box_blur_2.qml"
    gpuAlt: "movit.blur"
    keyframes {
        minimumVersion: '3'
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['hradius', 'vradius']
        parameters: [
            Parameter {
                name: qsTr('Horizontal')
                property: 'hradius'
                isCurve: true
                minimum: 0
                maximum: 100
            },
            Parameter {
                name: qsTr('Vertical')
                property: 'vradius'
                isCurve: true
                minimum: 0
                maximum: 100
            }
        ]
    }
}
