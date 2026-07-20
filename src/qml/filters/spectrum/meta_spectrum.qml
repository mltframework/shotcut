import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    objectName: 'audioSpectrum'
    name: qsTr('Audio Spectrum Visualization')
    keywords: qsTr('music visualizer reactive frequency', 'search keywords for the Audio Spectrum Visualization video filter') + ' audio spectrum visualization #rgba #10bit'
    mlt_service: 'audiospectrum'
    qml: 'ui_spectrum.qml'
    vui: 'vui_spectrum.qml'
    icon: 'icon.webp'
    allowMultiple: true
    help: 'https://forum.snapflow.org/t/audio-spectrum-visualization/12826/1'
}
