import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'fadeOutBrightness'
    name: qsTr("Fade Out Video")
    keywords: qsTr('brightness lightness opacity alpha', 'search keywords for the Fade Out video filter') + ' fade out video #rgba #yuv #10bit'
    mlt_service: "brightness"
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true
    gpuAlt: "movit.opacity"
    allowMultiple: false
    help: 'https://forum.shotcut.org/t/fade-out-video/12846/1'

    keyframes {
        allowTrim: false
        allowAnimateOut: true
    }
}
