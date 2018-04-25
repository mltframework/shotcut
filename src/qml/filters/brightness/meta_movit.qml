import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Brightness")
    objectName: "movitBrightness"
    mlt_service: "movit.opacity"
    needsGPU: true
    qml: "ui_movit.qml"
    isFavorite: true
    allowMultiple: false
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        parameters: [
            Parameter {
                name: qsTr('Brightness')
                property: 'opacity'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 2
            }
        ]
    }
}
