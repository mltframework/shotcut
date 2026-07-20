import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Track Seam")
    keywords: qsTr('click splice seam', 'search keywords for the Seam audio filter') + ' seam'
    mlt_service: "audioseam"
    objectName: 'audioSeam'
    qml: "ui.qml"
    isTrackOnly: true
    help: 'https://forum.snapflow.org/t/track-seam-audio-filter/40783/1'
}
