import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Glow")
    keywords: qsTr('shine blur', 'search keywords for the Glow video filter') + ' glow #rgba'
    mlt_service: "frei0r.glow"
    qml: "ui_frei0r.qml"
    icon: 'icon.webp'
    gpuAlt: "movit.glow"
    help: 'https://forum.shotcut.org/t/glow-video-filter/12849/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0']
        parameters: [
            Parameter {
                name: qsTr('Blur')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
