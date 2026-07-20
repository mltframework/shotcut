import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Link
    isAudio: true
    name: qsTr("Declick Audio")
    mlt_service: 'avfilter.adeclick'
    keywords: qsTr('declick crackle pop', 'search keywords for the Declick audio filter') + ' declick audio'
    objectName: 'audioDeclick'
    qml: 'ui.qml'
    help: 'https://forum.snapflow.org/t/declick-audio-filter/42273/1'
}
