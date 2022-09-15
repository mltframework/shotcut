import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'audioSpectrum'
    name: qsTr('Audio Spectrum Visualization')
    keywords: qsTr('music visualizer reactive frequency', 'search keywords for the Audio Spectrum Visualization video filter') + ' audio spectrum visualization'
    mlt_service: 'audiospectrum'
    qml: 'ui_spectrum.qml'
    vui: 'vui_spectrum.qml'
    icon: 'icon.webp'
    allowMultiple: true
}
