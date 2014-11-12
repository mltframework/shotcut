import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Mirror")
    mlt_service: "movit.mirror"
    needsGPU: true
    qml: "ui.qml"
}
