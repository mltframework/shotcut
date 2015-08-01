import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Reverb")
    mlt_service: 'ladspa.1216'
    qml: 'ui.qml'
}
