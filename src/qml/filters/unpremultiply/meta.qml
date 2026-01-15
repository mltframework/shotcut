import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Unpremultiply Alpha')
    keywords: qsTr('disassociate associated straight', 'search keywords for the Unpremultiply Alpha video filter') + ' unpremultiply alpha #rgba'
    mlt_service: 'frei0r.premultiply'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/unpremultiply-alpha/12891/1'
}
