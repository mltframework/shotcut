import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Flip")
    keywords: qsTr('vertical flop transpose rotate', 'search keywords for the Flip video filter') + ' flip'
    mlt_service: "avfilter.vflip"
    gpuAlt: "movit.flip"
    icon: 'icon.webp'
}
