import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Pan")
    keywords: qsTr('stereo balance channel mixer', 'search keywords for the Pan audio filter') + ' pan'
    mlt_service: 'panner'
    objectName: 'audioPan'
    qml: 'ui.qml'
    help: 'https://forum.snapflow.org/t/pan-audio-filter/12914/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['split']
        parameters: [
            Parameter {
                name: qsTr('Position')
                property: 'split'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
