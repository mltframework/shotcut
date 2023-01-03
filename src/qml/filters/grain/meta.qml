import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Old Film: Grain")
    keywords: qsTr('dots particles noise dirt', 'search keywords for the Old Film: Grain video filter') + ' old film: grain'
    mlt_service: "grain"
    qml: "ui.qml"
}
