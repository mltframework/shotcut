import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Hue/Lightness/Saturation")
    keywords: qsTr('color value desaturate grayscale', 'search keywords for the Hue/Lightness/Saturation video filter') + ' hue lightness saturation #yuv #10bit #color'
    mlt_service: 'avfilter.hue'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/hue-lightness-saturation/12852/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['av.h', 'av.b', 'av.s']
        parameters: [
            Parameter {
                name: qsTr('Hue')
                property: 'av.h'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Lightness')
                property: 'av.b'
                isCurve: true
                minimum: -10
                maximum: 10
            },
            Parameter {
                name: qsTr('Saturation')
                property: 'av.s'
                isCurve: true
                minimum: 0
                maximum: 5
            }
        ]
    }
}
