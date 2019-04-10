import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'audioLightshow'
    name: qsTr('Audio Light Visualization')
    mlt_service: 'lightshow'
    qml: 'ui_lightshow.qml'
    vui: 'vui.qml'
    allowMultiple: true
}
