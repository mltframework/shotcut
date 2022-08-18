import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Notch")
    keywords: qsTr('frequency pass', 'search keywords for the Notch audio filter') + ' notch'
    mlt_service: 'ladspa.1894'
    qml: 'ui.qml'
}
