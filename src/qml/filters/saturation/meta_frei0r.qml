import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Saturation")
    mlt_service: "frei0r.saturat0r"
    needsGPU: false
    qml: "ui_frei0r.qml"
}
