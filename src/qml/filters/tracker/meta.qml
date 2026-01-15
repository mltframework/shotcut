import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Motion Tracker')
    mlt_service: 'opencv.tracker'
    keywords: qsTr('tracking', 'search keywords for the Motion Tracker video filter') + ' motion tracker #rgb #10bit'
    qml: 'ui.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/motion-tracker-video-filter/40784/1'
}
