import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Halftone")
    objectName: 'halftone'
    mlt_service: "frei0r.colorhalftone"
    qml: "ui.qml"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1', '2', '3']
        parameters: [
            Parameter {
                name: qsTr('Radius')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Cyan')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Magenta')
                property: '2'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Yellow')
                property: '3'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }

}
