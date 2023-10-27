import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Link
    objectName: 'speedForwardReverse'
    isAudio: false
    name: qsTr("Speed: Forward & Reverse")
    keywords: qsTr('temporal speed ramp fast slow motion reverse', 'search keywords for the Speed filter') + ' speed'
    mlt_service: "timeremap"
    qml: "ui_forward_reverse.qml"
    isFavorite: false
    allowMultiple: false
    seekReverse: true
    minimumVersion: '2'

    keyframes {
        allowTrim: false
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['speed_map']
        parameters: [
            Parameter {
                name: qsTr('Speed')
                property: 'speed_map'
                isCurve: true
                rangeType: Parameter.MinMax
                minimum: -10
                maximum: 10
                units: 'x'
            }
        ]
    }
}
