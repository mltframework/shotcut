import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'richText'
    name: qsTr('Text: Rich')
    keywords: qsTr('type font format overlay', 'search keywords for the Text: Rich video filter') + ' html text: rich'
    mlt_service: 'qtext'
    qml: "ui.qml"
    vui: 'vui.qml'
    icon: 'icon.webp'
    isFavorite: true
    minimumVersion: '2'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['geometry']
        parameters: [
            Parameter {
                name: qsTr('Position / Size')
                property: 'geometry'
                isRectangle: true
            }
        ]
    }
}
