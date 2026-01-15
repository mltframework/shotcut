import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Contrast")
    keywords: qsTr('variation value', 'search keywords for the Contrast video filter') + ' contrast #rgba #color'
    objectName: "contrast"
    mlt_service: "lift_gamma_gain"
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true
    gpuAlt: "movit.lift_gamma_gain"
    help: 'https://forum.shotcut.org/t/contrast/12837/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['gain_r']
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: 'gain_r'
                gangedProperties: ['gain_g', 'gain_b', 'gamma_r', 'gamma_g', 'gamma_b']
            }
        ]
    }
}
