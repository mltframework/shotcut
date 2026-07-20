import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Reflect')
    keywords: qsTr('mirror repeat', 'search keywords for the Reflect video filter') + ' reflect #yuv'
    objectName: 'reflect'
    mlt_service: 'mirror'
    qml: "ui.qml"
    icon: 'icon.webp'
    help: 'https://forum.snapflow.org/t/reflect-video-filter/29279/1'
}
