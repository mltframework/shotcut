import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Flip")
    keywords: qsTr('vertical flop transpose rotate', 'search keywords for the Flip video filter') + ' flip'
    mlt_service: "movit.flip"
    needsGPU: true
}
