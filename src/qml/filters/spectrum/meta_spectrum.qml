import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'audioSpectrum'
    name: qsTr('Audio Spectrum Visualization')
    mlt_service: 'audiospectrum'
    qml: 'ui_spectrum.qml'
    vui: 'vui_spectrum.qml'
    allowMultiple: true
}
