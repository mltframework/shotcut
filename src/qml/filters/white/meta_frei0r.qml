import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("White Balance")
    mlt_service: "frei0r.colgate"
    qml: "ui.qml"
    isFavorite: true
    gpuAlt: "movit.white_balance"
}
