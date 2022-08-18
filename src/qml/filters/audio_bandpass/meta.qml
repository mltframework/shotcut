import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Band Pass")
    keywords: qsTr('frequency', 'search keywords for the Band Pass audio filter') + ' band pass'
    mlt_service: 'ladspa.1892'
    qml: 'ui.qml'
}
