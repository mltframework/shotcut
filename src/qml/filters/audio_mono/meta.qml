import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Downmix")
    keywords: qsTr('stereo mixdown channel', 'search keywords for the Downmix audio filter') + ' downmix'
    mlt_service: "mono"
}
