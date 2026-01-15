import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'gradientMap'
    name: qsTr('Gradient Map')
    keywords: qsTr('color mapping intensity', 'search keywords for the Gradient Map video filter') + ' gradient map gradientmap #rgba #color #10bit'
    mlt_service: 'gradientmap'
    qml: 'ui.qml'
    icon: 'icon.webp'
    allowMultiple: true
    help: 'https://forum.shotcut.org/t/gradient-map-video-filter/47075'
}
