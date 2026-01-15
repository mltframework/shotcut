import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Vignette")
    keywords: qsTr('dark edges fade', 'search keywords for the Vignette video filter') + ' vignette #yuv'
    mlt_service: "vignette"
    qml: "ui_oldfilm.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/vignette/12892/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['radius', 'smooth', 'opacity']
        parameters: [
            Parameter {
                name: qsTr('Radius')
                property: 'radius'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Feathering')
                property: 'smooth'
                isCurve: true
                minimum: 0
                maximum: 5
            },
            Parameter {
                name: qsTr('Opacity')
                property: 'opacity'
                isCurve: true
                minimum: 1
                maximum: 0
            }
        ]
    }
}
