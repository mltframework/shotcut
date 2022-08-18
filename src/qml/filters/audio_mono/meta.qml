import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Downmix")
    keywords: qsTr('stereo mixdown channel', 'search keywords for the Downmix audio filter') + ' downmix'
    mlt_service: "mono"
}
