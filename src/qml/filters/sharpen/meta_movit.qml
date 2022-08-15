import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Sharpen")
    mlt_service: "movit.sharpen"
    needsGPU: true
    qml: "ui_movit.qml"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['circle_radius', 'gaussian_radius', 'correlation', 'noise']
        parameters: [
            Parameter {
                name: qsTr('Circle radius')
                property: 'circle_radius'
                isCurve: true
                minimum: 0
                maximum: 99.99
            },
            Parameter {
                name: qsTr('Gaussian radius')
                property: 'gaussian_radius'
                isCurve: true
                minimum: 0
                maximum: 99.99
            },
            Parameter {
                name: qsTr('Correlation')
                property: 'correlation'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Noise')
                property: 'noise'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }

}
