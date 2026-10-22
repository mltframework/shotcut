/*
 * Copyright (c) 2026 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    color: activePalette.window
    width: 420
    height: 560

    function modeComboWidth() {
        let widest = "";
        for (let i = 0; i < modeCombo.model.length; ++i) {
            const entry = String(modeCombo.model[i]);
            if (entry.length > widest.length)
                widest = entry;
        }
        if (!widest)
            widest = "AI Chat";
        modeComboText.text = widest;
        return modeComboText.implicitWidth + 52;
    }

    SystemPalette {
        id: activePalette
    }

    Text {
        id: modeComboText
        visible: false
        font: modeCombo.font
        text: "AI Chat"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: coproducer.statusText
                elide: Text.ElideRight
                color: activePalette.text
                opacity: 0.85
            }

            ComboBox {
                id: modeCombo
                model: ["AI Chat", "Search User Guide"]
                currentIndex: coproducer.mode
                onCurrentIndexChanged: coproducer.mode = currentIndex
                Layout.minimumWidth: root.modeComboWidth()
                Layout.preferredWidth: root.modeComboWidth()
            }

            ToolButton {
                icon.name: "list-add"
                icon.source: "qrc:///icons/oxygen/32x32/actions/list-add.png"
                onClicked: coproducer.newChat()
            }

            ToolButton {
                icon.name: "show-menu"
                icon.source: "qrc:///icons/oxygen/32x32/actions/show-menu.png"
                onClicked: coproducer.openSettings()
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: transcriptView
                property bool autoScrollToBottom: false
                property bool finishScrollPending: false

                model: coproducer.transcript
                spacing: 8
                clip: true
                onCountChanged: if (count && autoScrollToBottom)
                    positionViewAtEnd()
                onContentHeightChanged: if (count && autoScrollToBottom)
                    positionViewAtEnd()
                onModelChanged: if (count && autoScrollToBottom)
                    positionViewAtEnd()

                Connections {
                    target: coproducer
                    function onBusyChanged() {
                        if (coproducer.busy) {
                            transcriptView.autoScrollToBottom = true;
                            transcriptView.finishScrollPending = false;
                        } else {
                            transcriptView.autoScrollToBottom = false;
                            transcriptView.finishScrollPending = true;
                            Qt.callLater(function () {
                                if (transcriptView.finishScrollPending && transcriptView.count > 0) {
                                    transcriptView.positionViewAtEnd();
                                    transcriptView.finishScrollPending = false;
                                }
                            });
                        }
                    }
                }

                delegate: Item {
                    width: ListView.view.width
                    height: bubble.implicitHeight + 12

                    property var item: modelData

                    Rectangle {
                        id: bubble

                        width: Math.min(parent.width * 0.82, bubbleText.implicitWidth + 24)
                        x: item.role === "user" ? parent.width - width - 12 : 12
                        y: 6
                        radius: 12
                        color: item.role === "user" ? activePalette.highlight : activePalette.alternateBase
                        border.color: activePalette.mid

                        implicitHeight: bubbleText.implicitHeight + 18

                        Text {
                            id: bubbleText
                            anchors.fill: parent
                            anchors.margins: 10
                            text: item.text
                            textFormat: Text.MarkdownText
                            wrapMode: Text.Wrap
                            font.pointSize: Math.max(8, Qt.application.font.pointSize - 2)
                            color: item.role === "user" ? activePalette.highlightedText : activePalette.text
                            linkColor: item.role === "user" ? activePalette.highlightedText : "#4da3ff"
                            onLinkActivated: Qt.openUrlExternally(link)
                        }

                        MouseArea {
                            id: linkMouseArea
                            anchors.fill: bubbleText
                            acceptedButtons: Qt.LeftButton
                            hoverEnabled: true
                            property string hoveredLink: ""
                            cursorShape: bubbleText.linkAt(mouseX, mouseY) ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onPositionChanged: mouse => {
                                const link = bubbleText.linkAt(mouse.x, mouse.y);
                                hoveredLink = link || "";
                                cursorShape = link ? Qt.PointingHandCursor : Qt.ArrowCursor;
                            }
                            onExited: {
                                hoveredLink = "";
                                cursorShape = Qt.ArrowCursor;
                            }
                            onClicked: mouse => {
                                const link = bubbleText.linkAt(mouse.x, mouse.y);
                                if (link)
                                    Qt.openUrlExternally(link);
                            }
                        }

                        ToolTip {
                            visible: linkMouseArea.containsMouse && linkMouseArea.hoveredLink.length > 0
                            text: linkMouseArea.hoveredLink
                            delay: 0
                            timeout: 3000
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextArea {
                id: promptBox
                Layout.fillWidth: true
                enabled: !coproducer.busy
                text: coproducer.promptText
                placeholderText: coproducer.mode === 1 ? qsTr("Enter keywords to search the User Guide...") : qsTr("Ask about Shotcut features and workflows. AI answers may be inaccurate.")
                placeholderTextColor: Qt.alpha(palette.text, 0.4)
                wrapMode: TextEdit.Wrap
                implicitHeight: 80

                Keys.onPressed: function (event) {
                    if (event.key === Qt.Key_Return && !(event.modifiers & Qt.ShiftModifier)) {
                        event.accepted = true;
                        coproducer.promptText = promptBox.text;
                        coproducer.submitPrompt();
                    }
                }

                onTextChanged: coproducer.promptText = text
            }

            Button {
                icon.name: coproducer.busy ? "window-close" : "go-up"
                icon.source: coproducer.busy ? "qrc:///icons/oxygen/32x32/actions/window-close.png" : "qrc:///icons/oxygen/32x32/actions/go-up.png"
                text: ""
                flat: true
                implicitWidth: 32
                implicitHeight: 32
                onClicked: coproducer.submitPrompt()
            }
        }
    }
}
