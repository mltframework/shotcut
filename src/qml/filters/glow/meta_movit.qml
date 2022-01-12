import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Glow")
    mlt_service: "movit.glow"
    needsGPU: true
    qml: "ui_movit.qml"
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
                maximum: 1.0
            }
        ]
    }
}
