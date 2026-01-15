import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("RGB Shift")
    keywords: qsTr('glitch chroma analog split', 'search keywords for the RGB Shift video filter') + ' rgb shift #rgba'
    mlt_service: "frei0r.rgbsplit0r"
    qml: "ui.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/rgb-shift/12875/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1']
        parameters: [
            Parameter {
                name: qsTr('Vertical')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Horizontal')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
