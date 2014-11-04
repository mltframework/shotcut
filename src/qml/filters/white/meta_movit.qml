import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("White Balance")
    mlt_service: "movit.white_balance"
    needsGPU: true
    qml: "ui.qml"
    isFavorite: true
}
