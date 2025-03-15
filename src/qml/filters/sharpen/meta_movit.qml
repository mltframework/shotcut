import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Sharpen")
    keywords: qsTr('sharpness focus clear crisp', 'search keywords for the Sharpen video filter') + ' sharpen #gpu'
    mlt_service: "movit.sharpen"
    needsGPU: true
    qml: "ui_movit.qml"
    icon: 'icon.webp'

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
                maximum: 10
            },
            Parameter {
                name: qsTr('Gaussian radius')
                property: 'gaussian_radius'
                isCurve: true
                minimum: 0
                maximum: 10
            },
            Parameter {
                name: qsTr('Correlation')
                property: 'correlation'
                isCurve: true
                minimum: 0
                maximum: 0.99
            },
            Parameter {
                name: qsTr('Noise')
                property: 'noise'
                isCurve: true
                minimum: 0.01
                maximum: 1
            }
        ]
    }
}
