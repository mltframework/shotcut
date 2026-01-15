import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Fisheye")
    keywords: qsTr('deform lens distort wide angle panoramic hemispherical', 'search keywords for the Fisheye video filter') + ' gopro fisheye #rgba'
    mlt_service: 'frei0r.defish0r'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/fisheye-video-filter/35894/1'
}
