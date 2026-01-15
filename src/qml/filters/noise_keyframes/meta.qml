import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Noise: Keyframes")
    keywords: qsTr('dirt grunge', 'search keywords for the Noise: Keyframes video filter') + ' noise: keyframes #rgba'
    objectName: 'noise_keyframes'
    mlt_service: "frei0r.rgbnoise"
    qml: "ui.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/noise-keyframes/12865/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0']
        parameters: [
            Parameter {
                name: qsTr('Amount')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
