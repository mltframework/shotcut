import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Reduce Noise: Smart Blur")
    keywords: qsTr('denoise artifact clean', 'search keywords for the Reduce Noise: Smart Blur video filter') + ' reduce noise: smart blur #yuv'
    mlt_service: 'avfilter.smartblur'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/reduce-noise-smart-blur/12874/1'
}
