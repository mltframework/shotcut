import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Blur")
    keywords: qsTr('soften obscure hide', 'search keywords for the Blur video filter') + ' blur'
    mlt_service: "movit.blur"
    needsGPU: true
    qml: "ui_movit.qml"
    icon: 'icon.webp'

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
