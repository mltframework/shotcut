import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    objectName: 'audioDance'
    name: qsTr('Audio Dance Visualization')
    keywords: qsTr('music visualizer reactive transform move size position rotate rotation', 'search keywords for the Audio Dance Visualization video filter') + ' audio dance visualization #rgba #10bit'
    mlt_service: 'dance'
    qml: 'ui_dance.qml'
    icon: 'icon.webp'
    allowMultiple: false
    help: 'https://forum.snapflow.org/t/audio-dance-visualization/12822/1'
}
