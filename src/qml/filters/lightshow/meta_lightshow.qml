import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'audioLightshow'
    name: qsTr('Audio Light Visualization')
    keywords: qsTr('music visualizer reactive color', 'search keywords for the Audio Light Visualization video filter') + ' audio light visualization'
    mlt_service: 'lightshow'
    qml: 'ui_lightshow.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'
    allowMultiple: true
}
