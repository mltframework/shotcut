import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'audioWaveform'
    name: qsTr('Audio Waveform Visualization')
    keywords: qsTr('music visualizer reactive', 'search keywords for the Audio Waveform Visualization video filter') + ' audio waveform visualization #rgba'
    mlt_service: 'audiowaveform'
    qml: 'ui.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'
    allowMultiple: true
}
