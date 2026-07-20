import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    objectName: 'gradientMap'
    name: qsTr('Gradient Map')
    keywords: qsTr('color mapping intensity', 'search keywords for the Gradient Map video filter') + ' gradient map gradientmap #rgba #color #10bit'
    mlt_service: 'gradientmap'
    qml: 'ui.qml'
    icon: 'icon.webp'
    allowMultiple: true
    help: 'https://forum.snapflow.org/t/gradient-map-video-filter/47075'
}
