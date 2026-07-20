import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Scan Lines')
    keywords: qsTr('analog horizontal television', 'search keywords for the Scan Lines video filter') + ' crt scan lines #rgba'
    mlt_service: 'frei0r.scanline0r'
    objectName: 'scanlines'
    icon: 'icon.webp'
    help: 'https://forum.snapflow.org/t/scan-lines-video-filter/14507/1'
}
