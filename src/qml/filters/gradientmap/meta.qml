import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'gradientMap'
    name: qsTr('Gradient Map')
    keywords: qsTr('color mapping intensity', 'search keywords for the Gradient Map video filter') + ' gradient map gradientmap'
    mlt_service: 'gradientmap'
    qml: 'ui.qml'
    icon: 'icon.webp'
    allowMultiple: true
}
