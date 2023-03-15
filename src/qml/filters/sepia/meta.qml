import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Sepia Tone")
    keywords: qsTr('color old photograph print', 'search keywords for the Sepia Tone video filter') + ' sepia tone'
    mlt_service: "sepia"
    qml: 'ui.qml'
    icon: 'icon.webp'
}
