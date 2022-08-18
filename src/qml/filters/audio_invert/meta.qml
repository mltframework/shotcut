import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr('Invert')
    keywords: qsTr('phase', 'search keywords for the Invert audio filter') + ' invert'
    mlt_service: 'ladspa.1429'
}
