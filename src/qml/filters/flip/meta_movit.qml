import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Flip")
    keywords: qsTr('vertical flop transpose rotate', 'search keywords for the Flip video filter') + ' flip #gpu #10bit'
    mlt_service: "movit.flip"
    needsGPU: true
    help: 'https://forum.shotcut.org/t/flip-video-filter/12847/1'
}
