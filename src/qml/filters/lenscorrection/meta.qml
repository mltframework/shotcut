import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Lens Correction")
    keywords: qsTr('deform lens distort wide angle panoramic hemispherical fisheye', 'search keywords for the Lens Correction video filter') + ' lens correction #rgba'
    mlt_service: 'frei0r.lenscorrection'
    qml: "ui.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/lens-correction/12856/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1', '2', '3']
        parameters: [
            Parameter {
                name: qsTr('X Center')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Y Center')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Correction at Center')
                property: '2'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Correction at Edges')
                property: '3'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
