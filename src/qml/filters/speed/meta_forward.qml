import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Link
    objectName: 'speedForward'
    isAudio: false
    name: qsTr("Speed: Forward Only")
    keywords: qsTr('temporal speed ramp fast slow motion', 'search keywords for the Speed filter') + ' speed'
    mlt_service: "timeremap"
    qml: "ui_forward.qml"
    isFavorite: false
    allowMultiple: false
    minimumVersion: '2'

    keyframes {
        allowTrim: false
        allowAnimateIn: false
        allowAnimateOut: false
        allowSmooth: false
        parameters: [
            Parameter {
                name: qsTr('Speed')
                property: 'speed_map'
                isCurve: true
                rangeType: Parameter.MinMax
                minimum: 0
                maximum: 10
                units: 'x'
            }
        ]
    }
}
