import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'audioLevelGraph'
    name: qsTr('Audio Level Visualization')
    mlt_service: 'audiolevelgraph'
    qml: 'ui.qml'
    vui: 'vui.qml'
    allowMultiple: true
}
