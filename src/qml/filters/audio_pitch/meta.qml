import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Pitch")
    mlt_service: "rbpitch"
    qml: "ui.qml"
    isFavorite: false
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['octaveshift']
        parameters: [
            Parameter {
                name: qsTr('Pitch')
                property: 'octaveshift'
                isCurve: true
                minimum: -2.0
                maximum: 2.0
            }
        ]
    }
}
