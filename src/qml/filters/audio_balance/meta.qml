import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Balance")
    keywords: qsTr('pan channel mixer fader', 'search keywords for the Balance audio filter') + ' balance'
    mlt_service: 'panner'
    objectName: 'audioBalance'
    qml: 'ui.qml'
    help: 'https://forum.shotcut.org/t/balance-audio-filter/12896/1'

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
