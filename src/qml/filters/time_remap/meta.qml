import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Link
    isAudio: false
    name: qsTr("Time Remap")
    mlt_service: "timeremap"
    qml: "ui.qml"
    isFavorite: false
    keyframes {
        parameters: [
            Parameter {
                name: qsTr('Map')
                property: 'map'
                isSimple: false
                isCurve: false
                minimum: 0
            }
        ]
    }
}
