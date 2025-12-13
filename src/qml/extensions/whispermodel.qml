import QtQuick
import org.shotcut.qml

Extension {
    id: whispermodel
    name: qsTr("Whisper Model")
    version: '1'
    files: [
        ExtensionFile {
            name: qsTr('Tiny Multilingual')
            description: qsTr('Tiny multilingual model')
            file: "ggml-tiny.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin"
            size: "81474355"
        },
        ExtensionFile {
            name: qsTr('Tiny Multilingual Quantized 5_1')
            description: qsTr('Tiny multilingual model quantized 5_1')
            file: "ggml-tiny-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny-q5_1.bin"
            size: "33764147"
        },
        ExtensionFile {
            name: qsTr('Tiny English')
            description: qsTr('Tiny english model')
            file: "ggml-tiny.en.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin"
            size: "81474355"
        },
        ExtensionFile {
            name: qsTr('Tiny English Quantized 5_1')
            description: qsTr('Tiny english model quantized 5_1')
            file: "ggml-tiny.en-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en-q5_1.bin"
            size: "33764147"
        },
        ExtensionFile {
            name: qsTr('Base Multilingual')
            description: qsTr('Base multilingual model')
            file: "ggml-base.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin"
            size: "155189248"
        },
        ExtensionFile {
            name: qsTr('Base Multilingual Quantized 5_1')
            description: qsTr('Base multilingual model quantized 5_1')
            file: "ggml-base-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base-q5_1.bin"
            size: "62599987"
            standard: true
        },
        ExtensionFile {
            name: qsTr('Base English')
            description: qsTr('Base english model')
            file: "ggml-base.en.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin"
            size: "155189248"
        },
        ExtensionFile {
            name: qsTr('Base English Quantized 5_1')
            description: qsTr('Base english model quantized 5_1')
            file: "ggml-base.en-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en-q5_1.bin"
            size: "62599987"
        },
        ExtensionFile {
            name: qsTr('Small Multilingual')
            description: qsTr('Small multilingual model')
            file: "ggml-small.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin"
            size: "511705088"
        },
        ExtensionFile {
            name: qsTr('Small Multilingual Quantized 5_1')
            description: qsTr('Small multilingual model quantized 5_1')
            file: "ggml-small-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small-q5_1.bin"
            size: "199229440"
        },
        ExtensionFile {
            name: qsTr('Small English')
            description: qsTr('Small english model')
            file: "ggml-small.en.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin"
            size: "511705088"
        },
        ExtensionFile {
            name: qsTr('Small English Quantized 5_1')
            description: qsTr('Small english model quantized 5_1')
            file: "ggml-small.en-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en-q5_1.bin"
            size: "199229440"
        },
        ExtensionFile {
            name: qsTr('Medium Multilingual')
            description: qsTr('Medium multilingual model')
            file: "ggml-medium.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin"
            size: "1604321280"
        },
        ExtensionFile {
            name: qsTr('Medium Multilingual Quantized 5_0')
            description: qsTr('Medium multilingual model quantized 5_0')
            file: "ggml-medium-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium-q5_0.bin"
            size: "565182464"
        },
        ExtensionFile {
            name: qsTr('Medium English')
            description: qsTr('Medium english model')
            file: "ggml-medium.en.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.en.bin"
            size: "1604321280"
        },
        ExtensionFile {
            name: qsTr('Medium English Quantized 5_0')
            description: qsTr('Medium english model quantized 5_0')
            file: "ggml-medium.en-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.en-q5_0.bin"
            size: "565182464"
        },
        ExtensionFile {
            name: qsTr('Large Multilingual (v3)')
            description: qsTr('Large multilingual model v3')
            file: "ggml-large-v3.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin"
            size: "3250585600"
        },
        ExtensionFile {
            name: qsTr('Large Multilingual (v3) Quantized 5_0')
            description: qsTr('Large multilingual model quantized 5_0')
            file: "ggml-large-v3-q5_0.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3-q5_0.bin"
            size: "1132462080"
        }
    ]
}
