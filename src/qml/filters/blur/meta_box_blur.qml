import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Blur: Box")
    keywords: qsTr('soften obscure hide directional', 'search keywords for the Blur: Box video filter') + ' blur: box'
    mlt_service: "box_blur"
    qml: "ui_box_blur.qml"
    icon: 'icon.webp'
    gpuAlt: "movit.blur"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['hradius', 'vradius']
        parameters: [
            Parameter {
                name: qsTr('Width')
                property: 'hradius'
                isCurve: true
                minimum: 0
                maximum: 100
            },
            Parameter {
                name: qsTr('Height')
                property: 'vradius'
                isCurve: true
                minimum: 0
                maximum: 100
            }
        ]
    }
}
