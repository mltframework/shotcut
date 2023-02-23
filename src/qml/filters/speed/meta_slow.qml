import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Link
    objectName: 'speedSlow'
    isAudio: false
    name: qsTr("Speed (Slow Motion)")
    keywords: qsTr('temporal speed ramp slow motion', 'search keywords for the Speed filter') + ' speed'
    mlt_service: "timeremap"
    qml: "ui_slow.qml"
    isFavorite: false
    allowMultiple: false

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
                maximum: 1
                units: 'x'
            }
        ]
    }

}
