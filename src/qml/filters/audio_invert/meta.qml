import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr('Invert')
    keywords: qsTr('phase', 'search keywords for the Invert audio filter') + ' invert'
    mlt_service: 'ladspa.1429'
}
