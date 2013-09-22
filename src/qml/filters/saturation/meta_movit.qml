import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Saturation")
    mlt_service: "movit.saturation"
    needsGPU: true
    qml: "ui_movit.qml"
}
