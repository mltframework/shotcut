import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'movitOpacity'
    name: qsTr("Opacity")
    mlt_service: "movit.opacity"
    needsGPU: true
    qml: "ui.qml"

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
