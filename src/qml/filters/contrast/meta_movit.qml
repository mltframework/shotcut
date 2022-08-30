import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Contrast")
    keywords: qsTr('variation value', 'search keywords for the Contrast video filter') + ' contrast'
    objectName: "movitContrast"
    mlt_service: "movit.lift_gamma_gain"
    needsGPU: true
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true

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
