import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Link
    isAudio: true
    name: qsTr("Reduce Noise: Audio (RNNoise)")
    mlt_service: 'rnnoise'
    keywords: qsTr('reduce noise denoise background wind speech clean', 'search keywords for the Reduce Noise: Audio filter') + ' reduce noise: audio rnnoise'
    objectName: 'audioRnnoiseLink'
    qml: 'ui.qml'
}
