import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'movitOpacity'
    name: qsTr("Opacity")
    keywords: qsTr('alpha transparent translucent', 'search keywords for the Opacity video filter') + ' opacity #gpu #10bit'
    mlt_service: "movit.opacity"
    needsGPU: true
    qml: "ui.qml"
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['opacity']
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: 'opacity'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
