import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    objectName: 'fadeOutVolume'
    name: qsTr("Fade Out Audio")
    keywords: qsTr('loudness', 'search keywords for the Fade Out audio filter') + ' fade out audio'
    mlt_service: "volume"
    qml: "ui.qml"
    isFavorite: true
    allowMultiple: false
    help: 'https://forum.shotcut.org/t/fade-out-audio/12905/1'

    keyframes {
        allowTrim: false
        allowAnimateOut: true
    }
}
