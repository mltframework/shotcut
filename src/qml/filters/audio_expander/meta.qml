import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Expander")
    keywords: qsTr('dynamics range', 'search keywords for the Expander audio filter') + ' expander'
    mlt_service: 'ladspa.1883'
    qml: 'ui.qml'
}
