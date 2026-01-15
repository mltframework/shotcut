import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Downmix")
    keywords: qsTr('stereo mixdown channel', 'search keywords for the Downmix audio filter') + ' downmix mono'
    mlt_service: "mono"
    qml: 'ui.qml'
    help: 'https://forum.shotcut.org/t/downmix-audio-filter/12902/1'
}
