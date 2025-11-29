import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("HSL Primaries")
    mlt_service: "hslprimaries"
    keywords: qsTr('hue saturation lightness color', 'search keywords for the HSL Primaries video filter') + ' hslprimaries #rgba #color #10bit'
    qml: "ui.qml"
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['overlap', 'h_shift_red', 's_scale_red', 'l_scale_red', 'h_shift_yellow', 's_scale_yellow', 'l_scale_yellow', 'h_shift_green', 's_scale_green', 'l_scale_green', 'h_shift_cyan', 's_scale_cyan', 'l_scale_cyan', 'h_shift_blue', 's_scale_blue', 'l_scale_blue', 'h_shift_magenta', 's_scale_magenta', 'l_scale_magenta']
        parameters: [
            Parameter {
                name: qsTr('Overlap')
                property: 'overlap'
                isCurve: true
                minimum: 0
                maximum: 100
            },
            Parameter {
                name: qsTr('Red Hue')
                property: 'h_shift_red'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Red Saturation')
                property: 's_scale_red'
                isCurve: true
                minimum: 0
                maximum: 500
            },
            Parameter {
                name: qsTr('Red Lightness')
                property: 'l_scale_red'
                isCurve: true
                minimum: 0
                maximum: 200
            },
            Parameter {
                name: qsTr('Yellow Hue')
                property: 'h_shift_yellow'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Yellow Saturation')
                property: 's_scale_yellow'
                isCurve: true
                minimum: 0
                maximum: 500
            },
            Parameter {
                name: qsTr('Yellow Lightness')
                property: 'l_scale_yellow'
                isCurve: true
                minimum: 0
                maximum: 200
            },
            Parameter {
                name: qsTr('Green Hue')
                property: 'h_shift_green'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Green Saturation')
                property: 's_scale_green'
                isCurve: true
                minimum: 0
                maximum: 500
            },
            Parameter {
                name: qsTr('Green Lightness')
                property: 'l_scale_green'
                isCurve: true
                minimum: 0
                maximum: 200
            },
            Parameter {
                name: qsTr('Cyan Hue')
                property: 'h_shift_cyan'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Cyan Saturation')
                property: 's_scale_cyan'
                isCurve: true
                minimum: 0
                maximum: 500
            },
            Parameter {
                name: qsTr('Cyan Lightness')
                property: 'l_scale_cyan'
                isCurve: true
                minimum: 0
                maximum: 200
            },
            Parameter {
                name: qsTr('Blue Hue')
                property: 'h_shift_blue'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Blue Saturation')
                property: 's_scale_blue'
                isCurve: true
                minimum: 0
                maximum: 500
            },
            Parameter {
                name: qsTr('Blue Lightness')
                property: 'l_scale_blue'
                isCurve: true
                minimum: 0
                maximum: 200
            },
            Parameter {
                name: qsTr('Magenta Hue')
                property: 'h_shift_magenta'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Magenta Saturation')
                property: 's_scale_magenta'
                isCurve: true
                minimum: 0
                maximum: 500
            },
            Parameter {
                name: qsTr('Magenta Lightness')
                property: 'l_scale_magenta'
                isCurve: true
                minimum: 0
                maximum: 200
            }
        ]
    }
}
