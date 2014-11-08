import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Glow")
    mlt_service: "frei0r.glow"
    qml: "ui_frei0r.qml"
    gpuAlt: "movit.glow"
}
