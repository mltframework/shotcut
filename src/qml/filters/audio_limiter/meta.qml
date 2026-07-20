import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Limiter")
    keywords: qsTr('dynamics range loudness', 'search keywords for the Limiter audio filter') + ' limiter'
    mlt_service: 'ladspa.1913'
    qml: 'ui.qml'
    help: 'https://forum.snapflow.org/t/limiter-audio-filter/12908/1'
}
