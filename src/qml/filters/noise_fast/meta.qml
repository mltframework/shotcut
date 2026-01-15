import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Noise: Fast")
    keywords: qsTr('dirt grunge', 'search keywords for the Noise: Fast video filter') + ' noise: fast #rgba #yuv'
    objectName: 'noise_fast'
    mlt_service: "avfilter.noise"
    qml: "ui.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/noise-fast/12864/1'
}
