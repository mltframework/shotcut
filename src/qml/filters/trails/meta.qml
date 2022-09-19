import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr('Trails')
    keywords: qsTr('temporal mix psychedelic motion blur', 'search keywords for the Trails video filter') + ' trails'
    mlt_service: 'avfilter.tmix'
    qml: 'ui.qml'
    icon: 'icon.webp'
}
