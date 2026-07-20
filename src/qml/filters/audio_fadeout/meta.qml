import QtQuick
import org.snapflow.qml

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
    help: 'https://forum.snapflow.org/t/fade-out-audio/12905/1'

    keyframes {
        allowTrim: false
        allowAnimateOut: true
    }
}
