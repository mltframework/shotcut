import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Glow")
    keywords: qsTr('shine blur', 'search keywords for the Glow video filter') + ' glow #gpu #10bit'
    mlt_service: "movit.glow"
    needsGPU: true
    qml: "ui_movit.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/glow-video-filter/12849/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['radius', 'blur_mix', 'highlight_cutoff']
        parameters: [
            Parameter {
                name: qsTr('Radius')
                property: 'radius'
                isCurve: true
                minimum: 0
                maximum: 100
            },
            Parameter {
                name: qsTr('Highlight blurriness')
                property: 'blur_mix'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Highlight cutoff')
                property: 'highlight_cutoff'
                isCurve: true
                minimum: 0.1
                maximum: 1
            }
        ]
    }
}
