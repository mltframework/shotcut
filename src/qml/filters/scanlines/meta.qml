import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr('Scan Lines')
    keywords: qsTr('analog horizontal television', 'search keywords for the Scan Lines video filter') + ' crt scan lines'
    mlt_service: 'frei0r.scanline0r'
    objectName: 'scanlines'
}
