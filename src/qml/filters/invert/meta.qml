import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Invert Colors")
    keywords: qsTr('reverse opposite negative', 'search keywords for the Invert Colors video filter') + ' invert colors'
    mlt_service: "invert"
    qml: "ui.qml"
    icon: 'icon.webp'
}
