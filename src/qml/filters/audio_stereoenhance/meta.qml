import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Stereo Enhancer")
    mlt_service: 'avfilter.haas'
    keywords: qsTr('channel spatial delay', 'search keywords for the Stereo Enhancer audio filter') + ' stereo enhancer'
    objectName: 'stereoEnhance'
    qml: 'ui.qml'
}
