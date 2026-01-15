import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Mosaic")
    keywords: qsTr('pixelize pixelate', 'search keywords for the Mosaic video filter') + ' mosaic #rgba'
    mlt_service: "frei0r.pixeliz0r"
    qml: "ui.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/mosaic/12863/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1']
        parameters: [
            Parameter {
                name: qsTr('Width')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 0.4
            },
            Parameter {
                name: qsTr('Height')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 0.4
            }
        ]
    }
}
