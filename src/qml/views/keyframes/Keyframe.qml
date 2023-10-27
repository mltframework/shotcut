/*
 * Copyright (c) 2018-2023 Meltytech, LLC
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
import "Keyframes.js" as Logic
import QtQuick
import QtQuick.Controls
import org.shotcut.qml

Rectangle {
    id: keyframeRoot

    property int position
    property int interpolation: KeyframesModel.DiscreteInterpolation // rectangle for discrete
    property int prevInterpolation: KeyframesModel.DiscreteInterpolation
    property bool isSelected: false
    property string name: ''
    property double value
    property int parameterIndex
    property bool isCurve: false
    property int trackHeight: Logic.trackHeight(isCurve)
    property double minimum: 0
    property double maximum: 1
    property double trackValue: (0.5 - (value - minimum) / (maximum - minimum)) * (trackHeight - height - 2 * border.width)
    property double minDragX: activeClip.x - width / 2
    property double maxDragX: activeClip.x + activeClip.width - width / 2
    property double minDragY: activeClip.y
    property double maxDragY: activeClip.y + activeClip.height - width
    property bool inRange: position >= (filter.in - producer.in) && position <= (filter.out - producer.in)

    signal clicked(var keyframe)
    signal rightClicked(var keyframe)

    onInterpolationChanged: canvas.requestPaint()
    onPrevInterpolationChanged: canvas.requestPaint()
    onIsSelectedChanged: canvas.requestPaint()

    function updateX() {
        x = position * timeScale - width / 2;
    }

    onPositionChanged: updateX()
    Component.onCompleted: {
        updateX();
    }
    anchors.verticalCenter: parameterRoot.verticalCenter
    anchors.verticalCenterOffset: isCurve ? minimum != maximum ? trackValue : 0 : 0
    height: 10
    width: height
    color: "transparent"
    opacity: inRange ? 1 : 0.3
    border.width: 0
    border.color: "transparent"

    SystemPalette {
        id: activePalette
    }

    Connections {
        function onTimeScaleChanged() {
            updateX();
        }

        target: root
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: {
            parent.clicked(keyframeRoot);
            parent.rightClicked(keyframeRoot);
        }
        hoverEnabled: true

        ToolTip {
            text: name
            visible: parent.containsMouse
            delay: mouseAreaLeft.pressed ? 0 : 1000
            timeout: mouseAreaLeft.pressed ? -1 : 5000
        }
    }

    MouseArea {
        id: mouseAreaLeft

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onClicked: producer.position = position
        onDoubleClicked: {
            parameters.remove(parameterIndex, index);
            root.selection = [];
        }

        onPressed: {
            parent.clicked(keyframeRoot);
            if (isCurve) {
                if (minimum == maximum) {
                    // Do not allow vertical dragging when there is no range to drag across
                    drag.minimumY = parent.y;
                    drag.maximumY = parent.y;
                } else {
                    drag.minimumY = minDragY;
                    drag.maximumY = maxDragY;
                }
            }
        }
        onEntered: {
            if (isCurve)
                parent.anchors.verticalCenter = undefined;
        }
        onReleased: {
            if (isCurve)
                parent.anchors.verticalCenter = parameterRoot.verticalCenter;
        }
        onPositionChanged: mouse => {
            if (isCurve) {
                if (mouse.modifiers & Qt.ControlModifier)
                    drag.axis = Drag.YAxis;
                else if (mouse.modifiers & Qt.AltModifier)
                    drag.axis = Drag.XAxis;
                else
                    drag.axis = Drag.XAndYAxis;
            }
            var keyX = parent.x + parent.width / 2;
            var cursorX = producer.position * timeScale;
            var newPosition = Math.round((keyX) / timeScale);
            var keyPosition = newPosition - (filter.in - producer.in);
            // Snap to cursor
            if (settings.timelineSnap && keyX > cursorX - 10 && keyX < cursorX + 10 && cursorX > minDragX + parent.width / 2 && cursorX < maxDragX + parent.width / 2 && drag.axis != Drag.YAxis) {
                keyPosition = Math.round((cursorX) / timeScale) - (filter.in - producer.in);
                parent.x = cursorX - (parent.width / 2);
            }
            var trackValue = Math.min(Math.max(0, 1 - parent.y / (parameterRoot.height - parent.height)), 1);
            trackValue = minimum + trackValue * (maximum - minimum);
            if (drag.axis === Drag.XAxis && newPosition !== keyframeRoot.position)
                parameters.setKeyframePosition(parameterIndex, index, keyPosition);
            else if (drag.axis === Drag.YAxis || (drag.axis === Drag.XAndYAxis && newPosition === keyframeRoot.position))
                parameters.setKeyframeValue(parameterIndex, index, trackValue);
            else if (drag.axis === Drag.XAndYAxis)
                parameters.setKeyframeValuePosition(parameterIndex, index, trackValue, keyPosition);
        }
        cursorShape: Qt.PointingHandCursor

        drag {
            target: parent
            axis: isCurve ? Drag.XAndYAxis : Drag.XAxis
            threshold: 0
            minimumX: minDragX
            maximumX: maxDragX
            minimumY: minDragY
            maximumY: maxDragY
        }
    }

    Canvas {
        id: canvas

        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            if (isSelected) {
                ctx.fillStyle = 'red';
            } else {
                ctx.fillStyle = activePalette.buttonText;
            }
            ctx.strokeStyle = activePalette.button;
            ctx.lineWidth = 1;
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.beginPath();
            context.moveTo(canvas.width / 2, 0);
            switch (prevInterpolation) {
            case KeyframesModel.DiscreteInterpolation:
                context.lineTo(0, 0);
                context.lineTo(0, canvas.height);
                context.lineTo(canvas.width / 2, canvas.height);
                break;
            default:
            case KeyframesModel.LinearInterpolation:
                context.lineTo(0, canvas.height / 2);
                context.lineTo(canvas.width / 2, canvas.height);
                break;
            case KeyframesModel.SmoothNaturalInterpolation:
            case KeyframesModel.SmoothLooseInterpolation:
            case KeyframesModel.SmoothTightInterpolation:
                context.arc(canvas.width / 2, canvas.height / 2, canvas.width / 2, 3 * Math.PI / 2, Math.PI / 2, true);
                break;
            case KeyframesModel.EaseInSinusoidal:
            case KeyframesModel.EaseInQuadratic:
            case KeyframesModel.EaseInCubic:
            case KeyframesModel.EaseInQuartic:
            case KeyframesModel.EaseInQuintic:
            case KeyframesModel.EaseInExponential:
            case KeyframesModel.EaseInCircular:
            case KeyframesModel.EaseInBack:
            case KeyframesModel.EaseInElastic:
            case KeyframesModel.EaseInBounce:
            case KeyframesModel.EaseOutSinusoidal:
            case KeyframesModel.EaseOutQuadratic:
            case KeyframesModel.EaseOutCubic:
            case KeyframesModel.EaseOutQuartic:
            case KeyframesModel.EaseOutQuintic:
            case KeyframesModel.EaseOutExponential:
            case KeyframesModel.EaseOutCircular:
            case KeyframesModel.EaseOutBack:
            case KeyframesModel.EaseOutElastic:
            case KeyframesModel.EaseOutBounce:
            case KeyframesModel.EaseInOutSinusoidal:
            case KeyframesModel.EaseInOutQuadratic:
            case KeyframesModel.EaseInOutCubic:
            case KeyframesModel.EaseInOutQuartic:
            case KeyframesModel.EaseInOutQuintic:
            case KeyframesModel.EaseInOutExponential:
            case KeyframesModel.EaseInOutCircular:
            case KeyframesModel.EaseInOutBack:
            case KeyframesModel.EaseInOutElastic:
            case KeyframesModel.EaseInOutBounce:
                context.lineTo(0, 0);
                context.lineTo(canvas.width * 0.3, canvas.height / 2);
                context.lineTo(0, canvas.height);
                context.lineTo(canvas.width / 2, canvas.height);
                break;
            }
            switch (interpolation) {
            case KeyframesModel.DiscreteInterpolation:
                context.lineTo(canvas.width, canvas.height);
                context.lineTo(canvas.width, 0);
                context.lineTo(canvas.width / 2, 0);
                break;
            default:
            case KeyframesModel.LinearInterpolation:
                context.lineTo(canvas.width, canvas.height / 2);
                context.lineTo(canvas.width / 2, 0);
                break;
            case KeyframesModel.SmoothNaturalInterpolation:
            case KeyframesModel.SmoothLooseInterpolation:
            case KeyframesModel.SmoothTightInterpolation:
                context.arc(canvas.width / 2, canvas.height / 2, canvas.width / 2, Math.PI / 2, 3 * Math.PI / 2, true);
                break;
            case KeyframesModel.EaseInSinusoidal:
            case KeyframesModel.EaseInQuadratic:
            case KeyframesModel.EaseInCubic:
            case KeyframesModel.EaseInQuartic:
            case KeyframesModel.EaseInQuintic:
            case KeyframesModel.EaseInExponential:
            case KeyframesModel.EaseInCircular:
            case KeyframesModel.EaseInBack:
            case KeyframesModel.EaseInElastic:
            case KeyframesModel.EaseInBounce:
            case KeyframesModel.EaseOutSinusoidal:
            case KeyframesModel.EaseOutQuadratic:
            case KeyframesModel.EaseOutCubic:
            case KeyframesModel.EaseOutQuartic:
            case KeyframesModel.EaseOutQuintic:
            case KeyframesModel.EaseOutExponential:
            case KeyframesModel.EaseOutCircular:
            case KeyframesModel.EaseOutBack:
            case KeyframesModel.EaseOutElastic:
            case KeyframesModel.EaseOutBounce:
            case KeyframesModel.EaseInOutSinusoidal:
            case KeyframesModel.EaseInOutQuadratic:
            case KeyframesModel.EaseInOutCubic:
            case KeyframesModel.EaseInOutQuartic:
            case KeyframesModel.EaseInOutQuintic:
            case KeyframesModel.EaseInOutExponential:
            case KeyframesModel.EaseInOutCircular:
            case KeyframesModel.EaseInOutBack:
            case KeyframesModel.EaseInOutElastic:
            case KeyframesModel.EaseInOutBounce:
                context.lineTo(canvas.width, canvas.height);
                context.lineTo(canvas.width * 0.7, canvas.height / 2);
                context.lineTo(canvas.width, 0);
                context.lineTo(canvas.width / 2, 0);
                break;
            }
            ctx.fill();
            ctx.stroke();
        }
    }
}
