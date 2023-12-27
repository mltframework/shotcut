import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Link
    name: qsTr("Declick Audio")
    mlt_service: 'avfilter.adeclick'
    keywords: qsTr('declick crackle pop', 'search keywords for the Declick audio filter') + ' declick audio'
    objectName: 'audioDeclick'
    qml: 'ui.qml'
}
