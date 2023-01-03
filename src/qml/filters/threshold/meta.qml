import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Threshold")
    keywords: qsTr('black white luma', 'search keywords for the Threshold video filter') + ' threshold'
    mlt_service: "threshold"
    qml: "ui.qml"
    icon: 'icon.webp'

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
