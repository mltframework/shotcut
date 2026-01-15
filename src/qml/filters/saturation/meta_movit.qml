import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Saturation")
    keywords: qsTr('color desaturate grayscale chroma', 'search keywords for the Saturation video filter') + ' saturation #gpu #10bit #color'
    mlt_service: "movit.saturation"
    needsGPU: true
    qml: "ui_movit.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/saturation/12878/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['saturation']
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: 'saturation'
                isCurve: true
                minimum: 0
                maximum: 3
            }
        ]
    }
}
