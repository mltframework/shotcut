import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Limiter")
    keywords: qsTr('dynamics range loudness', 'search keywords for the Limiter audio filter') + ' limiter'
    mlt_service: 'ladspa.1913'
    qml: 'ui.qml'
}
