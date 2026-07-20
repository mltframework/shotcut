import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    objectName: 'audioLightshow'
    name: qsTr('Audio Light Visualization')
    keywords: qsTr('music visualizer reactive color', 'search keywords for the Audio Light Visualization video filter') + ' audio light visualization #rgba #10bit'
    mlt_service: 'lightshow'
    qml: 'ui_lightshow.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'
    allowMultiple: true
    help: 'https://forum.snapflow.org/t/audio-light-visualization/12824/1'
}
