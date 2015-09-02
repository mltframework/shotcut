import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'muteVolume'
    isAudio: true
    name: qsTr("Mute")
    mlt_service: "volume"
    qml: "ui.qml"
    isFavorite: true
}
