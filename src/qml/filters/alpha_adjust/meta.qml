import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Alpha Channel: Adjust")
    keywords: qsTr('transparency shave shrink grow soft feather', 'search keywords for the Alpha Channel: Adjust video filter') + ' alpha channel: adjust #rgba #10bit'
    mlt_service: 'frei0r.alpha0ps'
    objectName: 'alphaChannelAdjust'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/alpha-channel-adjust/12577/1'
}
