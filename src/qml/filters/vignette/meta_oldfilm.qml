import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Vignette")
    keywords: qsTr('dark edges fade', 'search keywords for the Vignette video filter') + ' vignette'
    mlt_service: "vignette"
    qml: "ui_oldfilm.qml"
    gpuAlt: "movit.vignette"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['radius', 'smooth', 'opacity']
        minimumVersion: '1.0'
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
