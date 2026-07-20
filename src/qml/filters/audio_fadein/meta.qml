import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    objectName: 'fadeInVolume'
    name: qsTr("Fade In Audio")
    keywords: qsTr('loudness', 'search keywords for the Fade In audio filter') + ' fade in audio'
    mlt_service: "volume"
    qml: "ui.qml"
    isFavorite: true
    allowMultiple: false
    help: 'https://forum.snapflow.org/t/fade-in-audio/12904/1'

    keyframes {
        allowTrim: false
        allowAnimateIn: true
    }
}
