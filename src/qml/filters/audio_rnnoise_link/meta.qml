import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Link
    name: qsTr("Noise Reduction (RNNoise)")
    mlt_service: 'rnnoise'
    keywords: qsTr('noise reduction denoise background wind speech clean rnnoise', 'search keywords for the Noise Reduction (RNNoise) audio filter') + ' noise reduction denoise rnnoise'
    objectName: 'audioRnnoiseLink'
    qml: 'ui.qml'
    hidden: true
}
