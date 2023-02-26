import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Motion Tracker')
    mlt_service: 'opencv.tracker'
    keywords: qsTr('tracking', 'search keywords for the Motion Tracker video filter') + ' motion tracker'
    qml: 'ui.qml'
    vui: 'vui.qml'
}
