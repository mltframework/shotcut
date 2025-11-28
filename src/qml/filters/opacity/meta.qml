import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'brightnessOpacity'
    name: qsTr("Opacity")
    keywords: qsTr('alpha transparent translucent', 'search keywords for the Opacity video filter') + ' opacity #rgba #10bit'
    mlt_service: "brightness"
    qml: "ui.qml"
    icon: 'icon.webp'
    gpuAlt: "movit.opacity"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['alpha']
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: 'alpha'
                gangedProperties: ['opacity']
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
