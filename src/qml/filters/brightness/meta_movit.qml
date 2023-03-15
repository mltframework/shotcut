import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Brightness")
    keywords: qsTr('lightness value', 'search keywords for the Brightness video filter') + ' brightness gpu'
    objectName: "movitBrightness"
    mlt_service: "movit.opacity"
    needsGPU: true
    qml: "ui_movit.qml"
    icon: 'icon.webp'
    isFavorite: true

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['opacity']
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: 'opacity'
                isCurve: true
                minimum: 0
                maximum: 2
            }
        ]
    }
}
