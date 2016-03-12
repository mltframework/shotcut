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
}
