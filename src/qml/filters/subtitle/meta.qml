import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'subtitles'
    name: qsTr('Subtitle Burn In')
    keywords: qsTr('subtitle overlay burn', 'search keywords for the Subtitle Burn In video filter') + ' subtitle burn in #rgba'
    mlt_service: 'subtitle'
    qml: "ui.qml"
    vui: 'vui.qml'
    icon: 'icon.webp'
    isOutputOnly: true

    keyframes {
        allowAnimateIn: false
        allowAnimateOut: false
        simpleProperties: ['geometry', 'fgcolour', 'olcolour', 'bgcolour', 'opacity']
        parameters: [
            Parameter {
                name: qsTr('Position / Size')
                property: 'geometry'
                isRectangle: true
            },
            Parameter {
                name: qsTr('Font color')
                property: 'fgcolour'
                isCurve: false
                isColor: true
            },
            Parameter {
                name: qsTr('Outline')
                property: 'olcolour'
                isCurve: false
                isColor: true
            },
            Parameter {
                name: qsTr('Background')
                property: 'bgcolour'
                isCurve: false
                isColor: true
            },
            Parameter {
                name: qsTr('Opacity')
                property: 'opacity'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
