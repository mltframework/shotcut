import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Link
    isAudio: false
    name: qsTr("Time Remap")
    mlt_service: "timeremap"
    qml: "ui.qml"
    isFavorite: false
    allowMultiple: false

    keyframes {
        allowTrim: false
        allowAnimateIn: false
        allowAnimateOut: false
        allowSmooth: false
        parameters: [
            Parameter {
                name: qsTr('Time')
                property: 'map'
                isCurve: true
                rangeType: Parameter.ClipLength
                units: 's'
            }
        ]
    }

}
