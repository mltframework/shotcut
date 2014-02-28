import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    objectName: 'fadeOutVolume'
    name: qsTr("Fade Out")
    mlt_service: "volume"
    qml: "ui.qml"
}
