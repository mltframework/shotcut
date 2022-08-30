import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Color Grading")
    keywords: qsTr('correct shadows lift midtones gamma highlights gain hue lightness brightness value', 'search keywords for the Color Grading video filter') + ' color grading'
    mlt_service: "lift_gamma_gain"
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true
    gpuAlt: "movit.lift_gamma_gain"

    keyframes {
        parameters: [
            Parameter {
                name: qsTr('Shadows (Lift)')
                property: 'lift_r'
                gangedProperties: ['lift_g', 'lift_b']
            },
            Parameter {
                name: qsTr('Midtones (Gamma)')
                property: 'gamma_r'
                gangedProperties: ['gamma_g', 'gamma_b']
            },
            Parameter {
                name: qsTr('Highlights (Gain)')
                property: 'gain_r'
                gangedProperties: ['gain_g', 'gain_b']
            }
        ]
    }

}
