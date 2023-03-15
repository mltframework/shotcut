import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Contrast")
    keywords: qsTr('variation value', 'search keywords for the Contrast video filter') + ' contrast'
    objectName: "contrast"
    mlt_service: "lift_gamma_gain"
    qml: "ui.qml"
    icon: 'icon.webp'
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
