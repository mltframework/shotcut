import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Swap Channels")
    mlt_service: "channelcopy"
    keywords: qsTr('switch stereo', 'search keywords for the Swap Channels audio filter') + ' swap channels'
    objectName: 'audioSwapChannels'
    qml: "ui.qml"
}
