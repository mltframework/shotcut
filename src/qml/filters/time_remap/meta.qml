import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Link
    isAudio: false
    name: qsTr("Time Remap")
    keywords: qsTr('temporal speed ramp reverse fast slow motion', 'search keywords for the Time: Remap filter') + ' time: remap #rgba #yuv #10bit'
    mlt_service: "timeremap"
    qml: "ui.qml"
    isFavorite: false
    allowMultiple: false
    seekReverse: true

    keyframes {
        allowTrim: false
        allowAnimateIn: false
        allowAnimateOut: false
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
