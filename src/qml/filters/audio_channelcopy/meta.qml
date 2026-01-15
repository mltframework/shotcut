import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Copy Channel")
    keywords: qsTr('duplicate', 'search keywords for the Copy Channel audio filter') + ' copy channel'
    mlt_service: "channelcopy"
    objectName: 'audioChannelCopy'
    qml: 'ui.qml'
    help: 'https://forum.shotcut.org/t/copy-channel-audio-filter/12900/1'
}
