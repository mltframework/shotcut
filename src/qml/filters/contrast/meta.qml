import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Contrast")
    objectName: "contrast"
    mlt_service: "lift_gamma_gain"
    qml: "ui.qml"
    isFavorite: true
    gpuAlt: "movit.lift_gamma_gain"
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['gain_r']
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: 'gain_r'
                gangedProperties: ['gain_g', 'gain_b']
            }
        ]
    }
}
