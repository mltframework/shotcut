import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Reduce Noise: Wavelet')
    keywords: qsTr('vague denoise artifact dirt', 'search keywords for the Reduce Noise: Wavelet video filter') + ' reduce noise: wavelet #rgba #yuv #10bit'
    objectName: 'vaguedenoiser'
    mlt_service: 'avfilter.vaguedenoiser'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/reduce-noise-wavelet-video-filter/17789/1'
}
