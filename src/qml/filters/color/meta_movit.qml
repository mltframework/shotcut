import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Color Grading")
    keywords: qsTr('correct shadows lift midtones gamma highlights gain hue lightness brightness value', 'search keywords for the Color Grading video filter') + ' color grading #gpu #10bit #color'
    mlt_service: "movit.lift_gamma_gain"
    needsGPU: true
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true

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
