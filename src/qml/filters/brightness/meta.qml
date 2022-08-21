import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Brightness")
    mlt_service: "brightness"
    keywords: qsTr('lightness value', 'search keywords for the Brightness video filter') + ' brightness'
    qml: "ui.qml"
    isFavorite: true
    gpuAlt: "movit.opacity"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['level']
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: 'level'
                isCurve: true
                minimum: 0
                maximum: 2
            }
        ]
    }

}
