import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'movitOpacity'
    name: qsTr("Opacity")
    keywords: qsTr('alpha transparent translucent', 'search keywords for the Opacity video filter') + ' opacity #gpu #10bit'
    mlt_service: "movit.opacity"
    needsGPU: true
    qml: "ui.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/opacity/12871/1'

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
                maximum: filter ? (filter.isAtLeastVersion(5) ? 2 : 1) : 2
            }
        ]
    }
}
