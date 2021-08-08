import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Sharpen")
    mlt_service: "frei0r.sharpness"
    qml: "ui_frei0r.qml"
    gpuAlt: "movit.sharpen"
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1']
        parameters: [
            Parameter {
                name: qsTr('Amount')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Size')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
