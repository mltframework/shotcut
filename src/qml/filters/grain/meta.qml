import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Old Film: Grain")
    keywords: qsTr('dots particles noise dirt', 'search keywords for the Old Film: Grain video filter') + ' old film: grain'
    mlt_service: "grain"
    qml: "ui.qml"
}
