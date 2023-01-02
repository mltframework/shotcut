import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Vignette")
    keywords: qsTr('dark edges fade', 'search keywords for the Vignette video filter') + ' vignette'
    mlt_service: "movit.vignette"
    needsGPU: true
    qml: "ui_movit.qml"
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['radius', 'inner_radius']
        parameters: [
            Parameter {
                name: qsTr('Outer radius')
                property: 'radius'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Inner radius')
                property: 'inner_radius'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
