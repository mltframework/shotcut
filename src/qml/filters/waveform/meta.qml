import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'audioWaveform'
    name: qsTr('Audio Waveform Visualization')
    keywords: qsTr('music visualizer reactive', 'search keywords for the Audio Waveform Visualization video filter') + ' audio waveform visualization'
    mlt_service: 'audiowaveform'
    qml: 'ui.qml'
    vui: 'vui.qml'
    allowMultiple: true
}
