import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Mirror")
    keywords: qsTr('horizontal flip transpose flop', 'search keywords for the Mirror video filter') + ' mirror #rgba #yuv #10bit'
    mlt_service: "avfilter.hflip"
    gpuAlt: "movit.mirror"
    qml: "ui.qml"
    icon: 'icon.webp'
}
