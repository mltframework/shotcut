import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Trails')
    keywords: qsTr('temporal mix psychedelic motion blur', 'search keywords for the Trails video filter') + ' trails #rgba #yuv #10bit'
    mlt_service: 'avfilter.tmix'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.snapflow.org/t/trails-video-filter/14177/1'
}
