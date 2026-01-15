import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Compressor")
    keywords: qsTr('loudness dynamics range', 'search keywords for the Compressor audio filter') + ' compressor'
    mlt_service: 'ladspa.1882'
    qml: 'ui.qml'
    help: 'https://forum.shotcut.org/t/compressor-audio-filter/12899/1'
}
