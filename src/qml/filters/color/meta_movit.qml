import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Color Grading")
    mlt_service: "movit.lift_gamma_gain"
    needsGPU: true
    qml: "ui.qml"
    isFavorite: true
}
