import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Dither")
    objectName: 'dither'
    mlt_service: "frei0r.dither"
    qml: "ui.qml"
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1']
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
