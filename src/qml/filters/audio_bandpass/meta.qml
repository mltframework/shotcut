import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Band Pass")
    keywords: qsTr('frequency', 'search keywords for the Band Pass audio filter') + ' band pass'
    mlt_service: 'ladspa.1892'
    qml: 'ui.qml'
}
