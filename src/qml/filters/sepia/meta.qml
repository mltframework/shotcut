import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Sepia Tone")
    keywords: qsTr('color old photograph print', 'search keywords for the Sepia Tone video filter') + ' sepia tone #yuv #color'
    mlt_service: "sepia"
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/sepia-tone/12879/1'
}
