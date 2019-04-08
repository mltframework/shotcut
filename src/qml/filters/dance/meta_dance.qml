import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'audioDance'
    name: qsTr('Audio Dance Visualization')
    mlt_service: 'dance'
    qml: 'ui_dance.qml'
    allowMultiple: false
}
