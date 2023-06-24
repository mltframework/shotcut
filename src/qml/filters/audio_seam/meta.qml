import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Track Seam")
    keywords: qsTr('click splice seam', 'search keywords for the Seam audio filter') + ' seam'
    mlt_service: "audioseam"
    objectName: 'audioSeam'
    qml: "ui.qml"
    isTrackOnly: true
}
