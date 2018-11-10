import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'audioWaveform'
    name: qsTr('Audio Waveform Visualization')
    mlt_service: 'audiowaveform'
    qml: 'ui.qml'
    vui: 'vui.qml'
    allowMultiple: true
}
