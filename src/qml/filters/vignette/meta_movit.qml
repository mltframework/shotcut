import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Vignette")
    keywords: qsTr('dark edges fade', 'search keywords for the Vignette video filter') + ' vignette #gpu #10bit'
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
