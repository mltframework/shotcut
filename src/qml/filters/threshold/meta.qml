import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Threshold")
    keywords: qsTr('black white luma', 'search keywords for the Threshold video filter') + ' threshold'
    mlt_service: "threshold"
    qml: "ui.qml"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['midpoint']
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: 'midpoint'
                isCurve: true
                minimum: 0
                maximum: 255
            }
        ]
    }

}
