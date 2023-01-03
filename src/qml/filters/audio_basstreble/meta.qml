import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    isHidden: true
    name: qsTr("Bass & Treble")
    mlt_service: 'ladspa.1901'
    qml: 'ui.qml'
}
