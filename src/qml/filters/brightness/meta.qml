import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Brightness")
    mlt_service: "brightness"
    keywords: qsTr('lightness value exposure', 'search keywords for the Brightness video filter') + ' brightness #yuv'
    qml: "ui.qml"
    icon: 'icon.webp'
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
