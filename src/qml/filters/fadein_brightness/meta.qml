import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'fadeInBrightness'
    name: qsTr("Fade In Video")
    keywords: qsTr('brightness lightness opacity alpha', 'search keywords for the Fade In video filter') + ' fade in video #rgba #yuv #10bit'
    mlt_service: "brightness"
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true
    gpuAlt: "movit.opacity"
    allowMultiple: false
    help: 'https://forum.shotcut.org/t/fade-in-video/12845/1'

    keyframes {
        allowTrim: false
        allowAnimateIn: true
    }
}
