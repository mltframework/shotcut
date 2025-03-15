import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Trails')
    keywords: qsTr('temporal mix psychedelic motion blur', 'search keywords for the Trails video filter') + ' trails #rgba #yuv #10bit'
    mlt_service: 'avfilter.tmix'
    qml: 'ui.qml'
    icon: 'icon.webp'
}
