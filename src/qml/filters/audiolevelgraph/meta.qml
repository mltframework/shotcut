import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'audioLevelGraph'
    name: qsTr('Audio Level Visualization')
    keywords: qsTr('music visualizer reactive', 'search keywords for the Audio Level Visualization video filter') + ' audio level visualization #rgba'
    mlt_service: 'audiolevelgraph'
    qml: 'ui.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'
    allowMultiple: true
}
