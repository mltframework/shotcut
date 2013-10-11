import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Gain")
    mlt_service: "volume"
    qml: "ui.qml"
}
