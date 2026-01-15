import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Reduce Noise: Quantization")
    keywords: qsTr('denoise artifact postprocess compress', 'search keywords for the Reduce Noise: Quantization video filter') + ' fspp reduce noise: quantization #rgb #yuv'
    mlt_service: "avfilter.fspp"
    qml: "ui.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/reduce-noise-quantization-video-filter/50881/1'
}
