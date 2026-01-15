import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Blur")
    keywords: qsTr('soften obscure hide', 'search keywords for the Blur video filter') + ' blur #gpu #10bit'
    mlt_service: "movit.blur"
    needsGPU: true
    qml: "ui_movit.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/blur-gaussian/12830/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['radius']
        parameters: [
            Parameter {
                name: qsTr('Radius')
                property: 'radius'
                isCurve: true
                minimum: 0
                maximum: 99.99
            }
        ]
    }
}
