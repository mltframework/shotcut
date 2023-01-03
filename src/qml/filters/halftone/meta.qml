import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Halftone")
    keywords: qsTr('noise dots newsprint', 'search keywords for the Halftone video filter') + ' halftone'
    objectName: 'halftone'
    mlt_service: "frei0r.colorhalftone"
    qml: "ui.qml"
    icon: 'icon.webp'

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
