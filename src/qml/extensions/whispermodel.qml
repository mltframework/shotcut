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
            sha256: "be07e048e1e599ad46341c8d2a135645097a538221678b7acdd1b1919c6e1b21"
            size: "77691713"
        },
        ExtensionFile {
            name: qsTr('Tiny Multilingual Quantized 5_1')
            description: qsTr('Tiny multilingual model quantized 5_1')
            file: "ggml-tiny-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny-q5_1.bin"
            sha256: "818710568da3ca15689e31a743197b520007872ff9576237bda97bd1b469c3d7"
            size: "32152673"
        },
        ExtensionFile {
            name: qsTr('Tiny English')
            description: qsTr('Tiny english model')
            file: "ggml-tiny.en.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin"
            sha256: "921e4cf8686fdd993dcd081a5da5b6c365bfde1162e72b08d75ac75289920b1f"
            size: "77704715"
        },
        ExtensionFile {
            name: qsTr('Tiny English Quantized 5_1')
            description: qsTr('Tiny english model quantized 5_1')
            file: "ggml-tiny.en-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en-q5_1.bin"
            sha256: "c77c5766f1cef09b6b7d47f21b546cbddd4157886b3b5d6d4f709e91e66c7c2b"
            size: "32166155"
        },
        ExtensionFile {
            name: qsTr('Base Multilingual')
            description: qsTr('Base multilingual model')
            file: "ggml-base.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin"
            sha256: "60ed5bc3dd14eea856493d334349b405782ddcaf0028d4b5df4088345fba2efe"
            size: "147951465"
        },
        ExtensionFile {
            name: qsTr('Base Multilingual Quantized 5_1')
            description: qsTr('Base multilingual model quantized 5_1')
            file: "ggml-base-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base-q5_1.bin"
            sha256: "422f1ae452ade6f30a004d7e5c6a43195e4433bc370bf23fac9cc591f01a8898"
            size: "59707625"
            standard: true
        },
        ExtensionFile {
            name: qsTr('Base English')
            description: qsTr('Base english model')
            file: "ggml-base.en.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin"
            sha256: "a03779c86df3323075f5e796cb2ce5029f00ec8869eee3fdfb897afe36c6d002"
            size: "147964211"
        },
        ExtensionFile {
            name: qsTr('Base English Quantized 5_1')
            description: qsTr('Base english model quantized 5_1')
            file: "ggml-base.en-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en-q5_1.bin"
            sha256: "4baf70dd0d7c4247ba2b81fafd9c01005ac77c2f9ef064e00dcf195d0e2fdd2f"
            size: "59721011"
        },
        ExtensionFile {
            name: qsTr('Small Multilingual')
            description: qsTr('Small multilingual model')
            file: "ggml-small.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin"
            sha256: "1be3a9b2063867b937e64e2ec7483364a79917e157fa98c5d94b5c1fffea987b"
            size: "487601967"
        },
        ExtensionFile {
            name: qsTr('Small Multilingual Quantized 5_1')
            description: qsTr('Small multilingual model quantized 5_1')
            file: "ggml-small-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small-q5_1.bin"
            sha256: "ae85e4a935d7a567bd102fe55afc16bb595bdb618e11b2fc7591bc08120411bb"
            size: "190085487"
        },
        ExtensionFile {
            name: qsTr('Small English')
            description: qsTr('Small english model')
            file: "ggml-small.en.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin"
            sha256: "c6138d6d58ecc8322097e0f987c32f1be8bb0a18532a3f88f734d1bbf9c41e5d"
            size: "487614201"
        },
        ExtensionFile {
            name: qsTr('Small English Quantized 5_1')
            description: qsTr('Small english model quantized 5_1')
            file: "ggml-small.en-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en-q5_1.bin"
            sha256: "bfdff4894dcb76bbf647d56263ea2a96645423f1669176f4844a1bf8e478ad30"
            size: "190098681"
        },
        ExtensionFile {
            name: qsTr('Medium Multilingual')
            description: qsTr('Medium multilingual model')
            file: "ggml-medium.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin"
            sha256: "6c14d5adee5f86394037b4e4e8b59f1673b6cee10e3cf0b11bbdbee79c156208"
            size: "1533763059"
        },
        ExtensionFile {
            name: qsTr('Medium Multilingual Quantized 5_0')
            description: qsTr('Medium multilingual model quantized 5_0')
            file: "ggml-medium-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium-q5_0.bin"
            sha256: "19fea4b380c3a618ec4723c3eef2eb785ffba0d0538cf43f8f235e7b3b34220f"
            size: "539212467"
        },
        ExtensionFile {
            name: qsTr('Medium English')
            description: qsTr('Medium english model')
            file: "ggml-medium.en.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.en.bin"
            sha256: "cc37e93478338ec7700281a7ac30a10128929eb8f427dda2e865faa8f6da4356"
            size: "1533774781"
        },
        ExtensionFile {
            name: qsTr('Medium English Quantized 5_0')
            description: qsTr('Medium english model quantized 5_0')
            file: "ggml-medium.en-q5_1.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.en-q5_0.bin"
            sha256: "76733e26ad8fe1c7a5bf7531a9d41917b2adc0f20f2e4f5531688a8c6cd88eb0"
            size: "539225533"
        },
        ExtensionFile {
            name: qsTr('Large Multilingual (v3)')
            description: qsTr('Large multilingual model v3')
            file: "ggml-large-v3.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin"
            sha256: "64d182b440b98d5203c4f9bd541544d84c605196c4f7b845dfa11fb23594d1e2"
            size: "3095033483"
        },
        ExtensionFile {
            name: qsTr('Large Multilingual (v3) Quantized 5_0')
            description: qsTr('Large multilingual model quantized 5_0')
            file: "ggml-large-v3-q5_0.bin"
            url: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3-q5_0.bin"
            sha256: "d75795ecff3f83b5faa89d1900604ad8c780abd5739fae406de19f23ecd98ad1"
            size: "1081140203"
        }
    ]
}
