import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'brightnessOpacity'
    name: qsTr("Opacity")
    keywords: qsTr('alpha transparent translucent', 'search keywords for the Opacity video filter') + ' opacity'
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
