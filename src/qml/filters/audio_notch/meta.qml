import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Notch")
    keywords: qsTr('frequency pass', 'search keywords for the Notch audio filter') + ' notch'
    mlt_service: 'ladspa.1894'
    qml: 'ui.qml'
}
