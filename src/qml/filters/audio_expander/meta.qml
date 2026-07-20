import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Expander")
    keywords: qsTr('dynamics range', 'search keywords for the Expander audio filter') + ' expander'
    mlt_service: 'ladspa.1883'
    qml: 'ui.qml'
    help: 'https://forum.snapflow.org/t/expander-audio-filter/12903/1'
}
