import QtQuick
import org.shotcut.qml

Extension {
    id: llamamodel
    name: qsTr("Llama Model")
    version: "1"
    files: [
        // Tier 1: 2-3B parameters
        ExtensionFile {
            name: qsTr("Qwen3.5 2B Q4_K_M")
            description: qsTr("Newer, small general-purpose chat model (2B parameters)")
            file: "Qwen3.5-2B-Q4_K_M.gguf"
            url: "https://huggingface.co/unsloth/Qwen3.5-2B-GGUF/resolve/main/Qwen3.5-2B-Q4_K_M.gguf"
            sha256: "aaf42c8b7c3cab2bf3d69c355048d4a0ee9973d48f16c731c0520ee914699223"
            size: "1280835840"
            standard: true
        },
        ExtensionFile {
            name: qsTr("Llama 3.2 3B Instruct Q4_K_M")
            description: qsTr("Meta's small, capable general-purpose chat model (3B parameters)")
            file: "Llama-3.2-3B-Instruct-Q4_K_M.gguf"
            url: "https://huggingface.co/bartowski/Llama-3.2-3B-Instruct-GGUF/resolve/main/Llama-3.2-3B-Instruct-Q4_K_M.gguf"
            sha256: "6c1a2b41161032677be168d354123594c0e6e67d2b9227c84f296ad037c728ff"
            size: "2019377696"
        },

        // Tier 2: 4-5B parameters
        ExtensionFile {
            name: qsTr("Qwen3.5 4B Q4_K_M")
            description: qsTr("Small general-purpose chat model (4B parameters)")
            file: "Qwen3.5-4B-Q4_K_M.gguf"
            url: "https://huggingface.co/unsloth/Qwen3.5-4B-GGUF/resolve/main/Qwen3.5-4B-Q4_K_M.gguf"
            sha256: "00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4"
            size: "2740937888"
        },
        ExtensionFile {
            name: qsTr("Gemma 4 E2B Instruct Q4_K_M")
            description: qsTr("Google model; larger on disk than its \"E2B\" name suggests")
            file: "gemma-4-E2B-it-Q4_K_M.gguf"
            url: "https://huggingface.co/unsloth/gemma-4-E2B-it-GGUF/resolve/main/gemma-4-E2B-it-Q4_K_M.gguf"
            sha256: "740185b21d22ceb83a11c3aa62ad5842ef32c70f6096d756bbee85a1e4ec34b8"
            size: "3106738272"
        },

        // Tier 3: 7-8B parameters
        ExtensionFile {
            name: qsTr("Llama 3.1 8B Instruct Q4_K_M")
            description: qsTr("Meta's popular general-purpose chat model (8B parameters)")
            file: "Meta-Llama-3.1-8B-Instruct-Q4_K_M.gguf"
            url: "https://huggingface.co/bartowski/Meta-Llama-3.1-8B-Instruct-GGUF/resolve/main/Meta-Llama-3.1-8B-Instruct-Q4_K_M.gguf"
            sha256: "7b064f5842bf9532c91456deda288a1b672397a54fa729aa665952863033557c"
            size: "4920739232"
        },
        ExtensionFile {
            name: qsTr("Qwen3.5 9B Q4_K_M")
            description: qsTr("Medium chat model (9B parameters)")
            file: "Qwen3.5-9B-Q4_K_M.gguf"
            url: "https://huggingface.co/unsloth/Qwen3.5-9B-GGUF/resolve/main/Qwen3.5-9B-Q4_K_M.gguf"
            sha256: "03b74727a860a56338e042c4420bb3f04b2fec5734175f4cb9fa853daf52b7e8"
            size: "5680522464"
        },
        ExtensionFile {
            name: qsTr("Gemma 4 E4B Instruct Q4_K_M")
            description: qsTr("Google model; larger on disk than its \"E4B\" name suggests")
            file: "gemma-4-E4B-it-Q4_K_M.gguf"
            url: "https://huggingface.co/unsloth/gemma-4-E4B-it-GGUF/resolve/main/gemma-4-E4B-it-Q4_K_M.gguf"
            sha256: "85a896a047553e842f25297ee5b031d64ff30147d9c4af17b1e4b394cd1fab87"
            size: "4977171584"
        },

        // Tier 4: 12-14B parameters
        ExtensionFile {
            name: qsTr("Gemma 4 12B Instruct Q4_K_M")
            description: qsTr("High-quality Google model for more demanding tasks (12B parameters)")
            file: "gemma-4-12b-it-Q4_K_M.gguf"
            url: "https://huggingface.co/unsloth/gemma-4-12b-it-GGUF/resolve/main/gemma-4-12b-it-Q4_K_M.gguf"
            sha256: "0a270ec9fe6b34f4a0d33992b6135117b484ebc4766ab76b51d4ae8c457e4c42"
            size: "7121861440"
        },

        // Larger, non-tiered options
        ExtensionFile {
            name: qsTr("Qwen3.8 27B Q4_K_M")
            description: qsTr("High-quality Qwen model; needs a capable machine (27B parameters)")
            file: "Qwen3.8-27B-UD-Q4_K_M.gguf"
            url: "https://huggingface.co/unsloth/Qwen3.8-27B-GGUF/resolve/main/Qwen3.8-27B-UD-Q4_K_M.gguf"
            sha256: "322e194ff79741c7baa497c240f677f54b201b0efab44ca8e50f122b39123482"
            size: "16464440224"
        }
    ]
}
