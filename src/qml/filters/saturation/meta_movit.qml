import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Saturation")
    keywords: qsTr('color desaturate grayscale chroma', 'search keywords for the Saturation video filter') + ' saturation'
    mlt_service: "movit.saturation"
    needsGPU: true
    qml: "ui_movit.qml"

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
