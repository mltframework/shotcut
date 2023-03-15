import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'audioDance'
    name: qsTr('Audio Dance Visualization')
    keywords: qsTr('music visualizer reactive transform move size position rotate rotation', 'search keywords for the Audio Dance Visualization video filter') + ' audio dance visualization'
    mlt_service: 'dance'
    qml: 'ui_dance.qml'
    icon: 'icon.webp'
    allowMultiple: false
}
