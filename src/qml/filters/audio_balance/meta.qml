import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Balance")
    keywords: qsTr('pan channel mixer', 'search keywords for the Balance audio filter') + ' balance'
    mlt_service: 'panner'
    objectName: 'audioBalance'
    qml: 'ui.qml'

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
