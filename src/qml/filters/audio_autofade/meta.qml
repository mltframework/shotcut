import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Track Auto Fade")
    keywords: qsTr('click splice fade', 'search keywords for the Auto Fade audio filter') + ' auto fade'
    mlt_service: "autofade"
    objectName: 'autoFade'
    qml: "ui.qml"
    isTrackOnly: true
}
