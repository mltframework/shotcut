import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Stereo Enhancer")
    mlt_service: 'avfilter.haas'
    keywords: qsTr('channel spatial delay', 'search keywords for the Stereo Enhancer audio filter') + ' stereo enhancer'
    objectName: 'stereoEnhance'
    qml: 'ui.qml'
}
