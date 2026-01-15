import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'audioLevelGraph'
    name: qsTr('Audio Level Visualization')
    keywords: qsTr('music visualizer reactive', 'search keywords for the Audio Level Visualization video filter') + ' audio level visualization #rgba #10bit'
    mlt_service: 'audiolevelgraph'
    qml: 'ui.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'
    allowMultiple: true
    help: 'https://forum.shotcut.org/t/audio-level-visualization-video-filter/50889/1'
}
