import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Reflect')
    keywords: qsTr('mirror repeat', 'search keywords for the Reflect video filter') + ' reflect'
    objectName: 'reflect'
    mlt_service: 'mirror'
    qml: "ui.qml"
    icon: 'icon.webp'
}
