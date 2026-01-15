import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'brightnessOpacity'
    name: qsTr("Opacity")
    keywords: qsTr('alpha transparent translucent', 'search keywords for the Opacity video filter') + ' opacity #rgba #10bit'
    mlt_service: "brightness"
    qml: "ui.qml"
    icon: 'icon.webp'
    gpuAlt: "movit.opacity"
    help: 'https://forum.shotcut.org/t/opacity/12871/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['alpha']
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: 'alpha'
                gangedProperties: ['opacity']
                isCurve: true
                minimum: 0
                maximum: filter ? (filter.isAtLeastVersion(5) ? 2 : 1) : 2
            }
        ]
    }
}
