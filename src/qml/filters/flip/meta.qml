import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Flip")
    keywords: qsTr('vertical flop transpose rotate', 'search keywords for the Flip video filter') + ' flip #rgba #yuv #10bit'
    mlt_service: "avfilter.vflip"
    gpuAlt: "movit.flip"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/flip-video-filter/12847/1'
}
