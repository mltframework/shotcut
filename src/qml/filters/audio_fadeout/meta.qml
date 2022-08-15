import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    objectName: 'fadeOutVolume'
    name: qsTr("Fade Out Audio")
    mlt_service: "volume"
    qml: "ui.qml"
    isFavorite: true
    allowMultiple: false

    keyframes {
        allowTrim: false
        allowAnimateOut: true
    }

}
