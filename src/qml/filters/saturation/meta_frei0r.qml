import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Saturation")
    keywords: qsTr('color desaturate grayscale chroma', 'search keywords for the Saturation video filter') + ' saturation'
    mlt_service: "frei0r.saturat0r"
    qml: "ui_frei0r.qml"
    icon: 'icon.webp'
    gpuAlt: "movit.saturation"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0']
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 0.375
            }
        ]
    }

}
