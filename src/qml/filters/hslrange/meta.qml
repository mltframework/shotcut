import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("HSL Range")
    mlt_service: "hslrange"
    keywords: qsTr('hue saturation lightness color primaries', 'search keywords for the HSL Range video filter') + ' hslrange #rgba #color'
    qml: "ui.qml"
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['h_center', 'h_range', 'h_shift', 's_scale', 'l_scale', 'blend']
        parameters: [
            Parameter {
                name: qsTr('Blend')
                property: 'blend'
                isCurve: true
                minimum: 0
                maximum: 100
            },
            Parameter {
                name: qsTr('Hue Shift')
                property: 'h_shift'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Saturation Scale')
                property: 's_scale'
                isCurve: true
                minimum: 0
                maximum: 500
            },
            Parameter {
                name: qsTr('Lightness Scale')
                property: 'l_scale'
                isCurve: true
                minimum: 0
                maximum: 200
            },
            Parameter {
                name: qsTr('Hue Center')
                property: 'hue_center'
                isCurve: true
                minimum: 0
                maximum: 360
            },
            Parameter {
                name: qsTr('Hue Range')
                property: 'hue_range'
                isCurve: true
                minimum: 0
                maximum: 180
            }
        ]
    }
}
