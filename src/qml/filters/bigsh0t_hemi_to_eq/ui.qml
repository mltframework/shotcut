import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    property bool blockUpdate: true
    property double yawStart: 0
    property double yawMiddle: 0
    property double yawEnd: 0
    property double pitchStart: 0
    property double pitchMiddle: 0
    property double pitchEnd: 0
    property double rollStart: 0
    property double rollMiddle: 0
    property double rollEnd: 0
    property double frontXStart: 0
    property double frontXMiddle: 0
    property double frontXEnd: 0
    property double frontYStart: 0
    property double frontYMiddle: 0
    property double frontYEnd: 0
    property double frontUpStart: 0
    property double frontUpMiddle: 0
    property double frontUpEnd: 0
    property double backXStart: 0
    property double backXMiddle: 0
    property double backXEnd: 0
    property double backYStart: 0
    property double backYMiddle: 0
    property double backYEnd: 0
    property double backUpStart: 0
    property double backUpMiddle: 0
    property double backUpEnd: 0
    property double fovStart: 0
    property double fovMiddle: 0
    property double fovEnd: 0
    property double radiusStart: 0
    property double radiusMiddle: 0
    property double radiusEnd: 0
    property double nadirRadiusStart: 0
    property double nadirRadiusMiddle: 0
    property double nadirRadiusEnd: 0
    property double nadirCorrectionStartStart: 0
    property double nadirCorrectionStartMiddle: 0
    property double nadirCorrectionStartEnd: 0
    property int interpolationValue: 0
    property int projectionValue: 0

    function updateSimpleKeyframes() {
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            // When enabling simple keyframes, initialize the keyframes with the current value
            if (filter.keyframeCount("yaw") <= 0)
                yawStart = yawMiddle = yawEnd = filter.getDouble("yaw");
            if (filter.keyframeCount("pitch") <= 0)
                pitchStart = pitchMiddle = pitchEnd = filter.getDouble("pitch");
            if (filter.keyframeCount("roll") <= 0)
                rollStart = rollMiddle = rollEnd = filter.getDouble("roll");
            if (filter.keyframeCount("frontX") <= 0)
                frontXStart = frontXMiddle = frontXEnd = filter.getDouble("frontX");
            if (filter.keyframeCount("frontY") <= 0)
                frontYStart = frontYMiddle = frontYEnd = filter.getDouble("frontY");
            if (filter.keyframeCount("frontUp") <= 0)
                frontUpStart = frontUpMiddle = frontUpEnd = filter.getDouble("frontUp");
            if (filter.keyframeCount("backX") <= 0)
                backXStart = backXMiddle = backXEnd = filter.getDouble("backX");
            if (filter.keyframeCount("backY") <= 0)
                backYStart = backYMiddle = backYEnd = filter.getDouble("backY");
            if (filter.keyframeCount("backUp") <= 0)
                backUpStart = backUpMiddle = backUpEnd = filter.getDouble("backUp");
            if (filter.keyframeCount("fov") <= 0)
                fovStart = fovMiddle = fovEnd = filter.getDouble("fov");
            if (filter.keyframeCount("radius") <= 0)
                radiusStart = radiusMiddle = radiusEnd = filter.getDouble("radius");
            if (filter.keyframeCount("nadirRadius") <= 0)
                nadirRadiusStart = nadirRadiusMiddle = nadirRadiusEnd = filter.getDouble("nadirRadius");
            if (filter.keyframeCount("nadirCorrectionStart") <= 0)
                nadirCorrectionStartStart = nadirCorrectionStartMiddle = nadirCorrectionStartEnd = filter.getDouble("nadirCorrectionStart");
        }
        setControls();
        updateProperty_yaw(null);
        updateProperty_pitch(null);
        updateProperty_roll(null);
        updateProperty_frontX(null);
        updateProperty_frontY(null);
        updateProperty_frontUp(null);
        updateProperty_backX(null);
        updateProperty_backY(null);
        updateProperty_backUp(null);
        updateProperty_fov(null);
        updateProperty_radius(null);
        updateProperty_nadirRadius(null);
        updateProperty_nadirCorrectionStart(null);
        updateProperty_interpolation();
        updateProperty_projection();
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        yawSlider.value = filter.getDouble("yaw", position);
        yawKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("yaw") > 0;
        pitchSlider.value = filter.getDouble("pitch", position);
        pitchKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("pitch") > 0;
        rollSlider.value = filter.getDouble("roll", position);
        rollKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("roll") > 0;
        frontXSlider.value = filter.getDouble("frontX", position);
        frontXKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("frontX") > 0;
        frontYSlider.value = filter.getDouble("frontY", position);
        frontYKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("frontY") > 0;
        frontUpSlider.value = filter.getDouble("frontUp", position);
        frontUpKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("frontUp") > 0;
        backXSlider.value = filter.getDouble("backX", position);
        backXKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("backX") > 0;
        backYSlider.value = filter.getDouble("backY", position);
        backYKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("backY") > 0;
        backUpSlider.value = filter.getDouble("backUp", position);
        backUpKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("backUp") > 0;
        fovSlider.value = filter.getDouble("fov", position);
        fovKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("fov") > 0;
        radiusSlider.value = filter.getDouble("radius", position);
        radiusKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("radius") > 0;
        nadirRadiusSlider.value = filter.getDouble("nadirRadius", position);
        nadirRadiusKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("nadirRadius") > 0;
        nadirCorrectionStartSlider.value = filter.getDouble("nadirCorrectionStart", position);
        nadirCorrectionStartKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("nadirCorrectionStart") > 0;
        interpolationComboBox.currentIndex = filter.get("interpolation");
        projectionComboBox.currentIndex = filter.get("projection");
        blockUpdate = false;
    }

    function updateProperty_yaw(position) {
        if (blockUpdate)
            return;
        var value = yawSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                yawStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                yawEnd = value;
            else
                yawMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("yaw");
            yawKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("yaw", yawStart, 0);
                filter.set("yaw", yawMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("yaw", yawMiddle, filter.duration - filter.animateOut);
                filter.set("yaw", yawEnd, filter.duration - 1);
            }
        } else if (!yawKeyframesButton.checked) {
            filter.resetProperty("yaw");
            filter.set("yaw", yawMiddle);
        } else if (position !== null) {
            filter.set("yaw", value, position);
        }
    }

    function updateProperty_pitch(position) {
        if (blockUpdate)
            return;
        var value = pitchSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                pitchStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                pitchEnd = value;
            else
                pitchMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("pitch");
            pitchKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("pitch", pitchStart, 0);
                filter.set("pitch", pitchMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("pitch", pitchMiddle, filter.duration - filter.animateOut);
                filter.set("pitch", pitchEnd, filter.duration - 1);
            }
        } else if (!pitchKeyframesButton.checked) {
            filter.resetProperty("pitch");
            filter.set("pitch", pitchMiddle);
        } else if (position !== null) {
            filter.set("pitch", value, position);
        }
    }

    function updateProperty_roll(position) {
        if (blockUpdate)
            return;
        var value = rollSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                rollStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                rollEnd = value;
            else
                rollMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("roll");
            rollKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("roll", rollStart, 0);
                filter.set("roll", rollMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("roll", rollMiddle, filter.duration - filter.animateOut);
                filter.set("roll", rollEnd, filter.duration - 1);
            }
        } else if (!rollKeyframesButton.checked) {
            filter.resetProperty("roll");
            filter.set("roll", rollMiddle);
        } else if (position !== null) {
            filter.set("roll", value, position);
        }
    }

    function updateProperty_frontX(position) {
        if (blockUpdate)
            return;
        var value = frontXSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                frontXStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                frontXEnd = value;
            else
                frontXMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("frontX");
            frontXKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("frontX", frontXStart, 0);
                filter.set("frontX", frontXMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("frontX", frontXMiddle, filter.duration - filter.animateOut);
                filter.set("frontX", frontXEnd, filter.duration - 1);
            }
        } else if (!frontXKeyframesButton.checked) {
            filter.resetProperty("frontX");
            filter.set("frontX", frontXMiddle);
        } else if (position !== null) {
            filter.set("frontX", value, position);
        }
    }

    function updateProperty_frontY(position) {
        if (blockUpdate)
            return;
        var value = frontYSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                frontYStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                frontYEnd = value;
            else
                frontYMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("frontY");
            frontYKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("frontY", frontYStart, 0);
                filter.set("frontY", frontYMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("frontY", frontYMiddle, filter.duration - filter.animateOut);
                filter.set("frontY", frontYEnd, filter.duration - 1);
            }
        } else if (!frontYKeyframesButton.checked) {
            filter.resetProperty("frontY");
            filter.set("frontY", frontYMiddle);
        } else if (position !== null) {
            filter.set("frontY", value, position);
        }
    }

    function updateProperty_frontUp(position) {
        if (blockUpdate)
            return;
        var value = frontUpSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                frontUpStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                frontUpEnd = value;
            else
                frontUpMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("frontUp");
            frontUpKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("frontUp", frontUpStart, 0);
                filter.set("frontUp", frontUpMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("frontUp", frontUpMiddle, filter.duration - filter.animateOut);
                filter.set("frontUp", frontUpEnd, filter.duration - 1);
            }
        } else if (!frontUpKeyframesButton.checked) {
            filter.resetProperty("frontUp");
            filter.set("frontUp", frontUpMiddle);
        } else if (position !== null) {
            filter.set("frontUp", value, position);
        }
    }

    function updateProperty_backX(position) {
        if (blockUpdate)
            return;
        var value = backXSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                backXStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                backXEnd = value;
            else
                backXMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("backX");
            backXKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("backX", backXStart, 0);
                filter.set("backX", backXMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("backX", backXMiddle, filter.duration - filter.animateOut);
                filter.set("backX", backXEnd, filter.duration - 1);
            }
        } else if (!backXKeyframesButton.checked) {
            filter.resetProperty("backX");
            filter.set("backX", backXMiddle);
        } else if (position !== null) {
            filter.set("backX", value, position);
        }
    }

    function updateProperty_backY(position) {
        if (blockUpdate)
            return;
        var value = backYSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                backYStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                backYEnd = value;
            else
                backYMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("backY");
            backYKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("backY", backYStart, 0);
                filter.set("backY", backYMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("backY", backYMiddle, filter.duration - filter.animateOut);
                filter.set("backY", backYEnd, filter.duration - 1);
            }
        } else if (!backYKeyframesButton.checked) {
            filter.resetProperty("backY");
            filter.set("backY", backYMiddle);
        } else if (position !== null) {
            filter.set("backY", value, position);
        }
    }

    function updateProperty_backUp(position) {
        if (blockUpdate)
            return;
        var value = backUpSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                backUpStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                backUpEnd = value;
            else
                backUpMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("backUp");
            backUpKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("backUp", backUpStart, 0);
                filter.set("backUp", backUpMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("backUp", backUpMiddle, filter.duration - filter.animateOut);
                filter.set("backUp", backUpEnd, filter.duration - 1);
            }
        } else if (!backUpKeyframesButton.checked) {
            filter.resetProperty("backUp");
            filter.set("backUp", backUpMiddle);
        } else if (position !== null) {
            filter.set("backUp", value, position);
        }
    }

    function updateProperty_fov(position) {
        if (blockUpdate)
            return;
        var value = fovSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                fovStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                fovEnd = value;
            else
                fovMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("fov");
            fovKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("fov", fovStart, 0);
                filter.set("fov", fovMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("fov", fovMiddle, filter.duration - filter.animateOut);
                filter.set("fov", fovEnd, filter.duration - 1);
            }
        } else if (!fovKeyframesButton.checked) {
            filter.resetProperty("fov");
            filter.set("fov", fovMiddle);
        } else if (position !== null) {
            filter.set("fov", value, position);
        }
    }

    function updateProperty_radius(position) {
        if (blockUpdate)
            return;
        var value = radiusSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                radiusStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                radiusEnd = value;
            else
                radiusMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("radius");
            radiusKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("radius", radiusStart, 0);
                filter.set("radius", radiusMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("radius", radiusMiddle, filter.duration - filter.animateOut);
                filter.set("radius", radiusEnd, filter.duration - 1);
            }
        } else if (!radiusKeyframesButton.checked) {
            filter.resetProperty("radius");
            filter.set("radius", radiusMiddle);
        } else if (position !== null) {
            filter.set("radius", value, position);
        }
    }

    function updateProperty_nadirRadius(position) {
        if (blockUpdate)
            return;
        var value = nadirRadiusSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                nadirRadiusStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                nadirRadiusEnd = value;
            else
                nadirRadiusMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("nadirRadius");
            nadirRadiusKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("nadirRadius", nadirRadiusStart, 0);
                filter.set("nadirRadius", nadirRadiusMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("nadirRadius", nadirRadiusMiddle, filter.duration - filter.animateOut);
                filter.set("nadirRadius", nadirRadiusEnd, filter.duration - 1);
            }
        } else if (!nadirRadiusKeyframesButton.checked) {
            filter.resetProperty("nadirRadius");
            filter.set("nadirRadius", nadirRadiusMiddle);
        } else if (position !== null) {
            filter.set("nadirRadius", value, position);
        }
    }

    function updateProperty_nadirCorrectionStart(position) {
        if (blockUpdate)
            return;
        var value = nadirCorrectionStartSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                nadirCorrectionStartStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                nadirCorrectionStartEnd = value;
            else
                nadirCorrectionStartMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("nadirCorrectionStart");
            nadirCorrectionStartKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("nadirCorrectionStart", nadirCorrectionStartStart, 0);
                filter.set("nadirCorrectionStart", nadirCorrectionStartMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("nadirCorrectionStart", nadirCorrectionStartMiddle, filter.duration - filter.animateOut);
                filter.set("nadirCorrectionStart", nadirCorrectionStartEnd, filter.duration - 1);
            }
        } else if (!nadirCorrectionStartKeyframesButton.checked) {
            filter.resetProperty("nadirCorrectionStart");
            filter.set("nadirCorrectionStart", nadirCorrectionStartMiddle);
        } else if (position !== null) {
            filter.set("nadirCorrectionStart", value, position);
        }
    }

    function updateProperty_interpolation() {
        if (blockUpdate)
            return;
        var value = interpolationComboBox.currentIndex;
        filter.set("interpolation", value);
    }

    function updateProperty_projection() {
        if (blockUpdate)
            return;
        var value = projectionComboBox.currentIndex;
        filter.set("projection", value);
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    width: 350
    height: 550
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set("yaw", 0);
        } else {
            yawMiddle = filter.getDouble("yaw", filter.animateIn);
            if (filter.animateIn > 0)
                yawStart = filter.getDouble("yaw", 0);
            if (filter.animateOut > 0)
                yawEnd = filter.getDouble("yaw", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("pitch", 0);
        } else {
            pitchMiddle = filter.getDouble("pitch", filter.animateIn);
            if (filter.animateIn > 0)
                pitchStart = filter.getDouble("pitch", 0);
            if (filter.animateOut > 0)
                pitchEnd = filter.getDouble("pitch", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("roll", 0);
        } else {
            rollMiddle = filter.getDouble("roll", filter.animateIn);
            if (filter.animateIn > 0)
                rollStart = filter.getDouble("roll", 0);
            if (filter.animateOut > 0)
                rollEnd = filter.getDouble("roll", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("frontX", 0.75);
        } else {
            frontXMiddle = filter.getDouble("frontX", filter.animateIn);
            if (filter.animateIn > 0)
                frontXStart = filter.getDouble("frontX", 0);
            if (filter.animateOut > 0)
                frontXEnd = filter.getDouble("frontX", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("frontY", 0.5);
        } else {
            frontYMiddle = filter.getDouble("frontY", filter.animateIn);
            if (filter.animateIn > 0)
                frontYStart = filter.getDouble("frontY", 0);
            if (filter.animateOut > 0)
                frontYEnd = filter.getDouble("frontY", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("frontUp", 90);
        } else {
            frontUpMiddle = filter.getDouble("frontUp", filter.animateIn);
            if (filter.animateIn > 0)
                frontUpStart = filter.getDouble("frontUp", 0);
            if (filter.animateOut > 0)
                frontUpEnd = filter.getDouble("frontUp", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("backX", 0.25);
        } else {
            backXMiddle = filter.getDouble("backX", filter.animateIn);
            if (filter.animateIn > 0)
                backXStart = filter.getDouble("backX", 0);
            if (filter.animateOut > 0)
                backXEnd = filter.getDouble("backX", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("backY", 0.5);
        } else {
            backYMiddle = filter.getDouble("backY", filter.animateIn);
            if (filter.animateIn > 0)
                backYStart = filter.getDouble("backY", 0);
            if (filter.animateOut > 0)
                backYEnd = filter.getDouble("backY", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("backUp", 270);
        } else {
            backUpMiddle = filter.getDouble("backUp", filter.animateIn);
            if (filter.animateIn > 0)
                backUpStart = filter.getDouble("backUp", 0);
            if (filter.animateOut > 0)
                backUpEnd = filter.getDouble("backUp", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("fov", 180);
        } else {
            fovMiddle = filter.getDouble("fov", filter.animateIn);
            if (filter.animateIn > 0)
                fovStart = filter.getDouble("fov", 0);
            if (filter.animateOut > 0)
                fovEnd = filter.getDouble("fov", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("radius", 0.25);
        } else {
            radiusMiddle = filter.getDouble("radius", filter.animateIn);
            if (filter.animateIn > 0)
                radiusStart = filter.getDouble("radius", 0);
            if (filter.animateOut > 0)
                radiusEnd = filter.getDouble("radius", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("nadirRadius", 0.2229);
        } else {
            nadirRadiusMiddle = filter.getDouble("nadirRadius", filter.animateIn);
            if (filter.animateIn > 0)
                nadirRadiusStart = filter.getDouble("nadirRadius", 0);
            if (filter.animateOut > 0)
                nadirRadiusEnd = filter.getDouble("nadirRadius", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("nadirCorrectionStart", 0.8);
        } else {
            nadirCorrectionStartMiddle = filter.getDouble("nadirCorrectionStart", filter.animateIn);
            if (filter.animateIn > 0)
                nadirCorrectionStartStart = filter.getDouble("nadirCorrectionStart", 0);
            if (filter.animateOut > 0)
                nadirCorrectionStartEnd = filter.getDouble("nadirCorrectionStart", filter.duration - 1);
        }
        if (filter.isNew)
            filter.set("interpolation", 1);
        else
            interpolationValue = filter.get("interpolation");
        if (filter.isNew)
            filter.set("projection", 0);
        else
            projectionValue = filter.get("projection");
        if (filter.isNew)
            filter.savePreset(preset.parameters);
        setControls();
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: ["yaw", "pitch", "roll", "frontX", "frontY", "frontUp", "backX", "backY", "backUp", "fov", "radius", "nadirRadius", "nadirCorrectionStart", "interpolation", "projection"]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty('yaw');
                filter.resetProperty('pitch');
                filter.resetProperty('roll');
                filter.resetProperty('frontX');
                filter.resetProperty('frontY');
                filter.resetProperty('frontUp');
                filter.resetProperty('backX');
                filter.resetProperty('backY');
                filter.resetProperty('backUp');
                filter.resetProperty('fov');
                filter.resetProperty('radius');
                filter.resetProperty('nadirRadius');
                filter.resetProperty('nadirCorrectionStart');
                filter.resetProperty('interpolation');
                filter.resetProperty('projection');
            }
            onPresetSelected: {
                yawMiddle = filter.getDouble("yaw", filter.animateIn);
                if (filter.animateIn > 0)
                    yawStart = filter.getDouble("yaw", 0);
                if (filter.animateOut > 0)
                    yawEnd = filter.getDouble("yaw", filter.duration - 1);
                pitchMiddle = filter.getDouble("pitch", filter.animateIn);
                if (filter.animateIn > 0)
                    pitchStart = filter.getDouble("pitch", 0);
                if (filter.animateOut > 0)
                    pitchEnd = filter.getDouble("pitch", filter.duration - 1);
                rollMiddle = filter.getDouble("roll", filter.animateIn);
                if (filter.animateIn > 0)
                    rollStart = filter.getDouble("roll", 0);
                if (filter.animateOut > 0)
                    rollEnd = filter.getDouble("roll", filter.duration - 1);
                frontXMiddle = filter.getDouble("frontX", filter.animateIn);
                if (filter.animateIn > 0)
                    frontXStart = filter.getDouble("frontX", 0);
                if (filter.animateOut > 0)
                    frontXEnd = filter.getDouble("frontX", filter.duration - 1);
                frontYMiddle = filter.getDouble("frontY", filter.animateIn);
                if (filter.animateIn > 0)
                    frontYStart = filter.getDouble("frontY", 0);
                if (filter.animateOut > 0)
                    frontYEnd = filter.getDouble("frontY", filter.duration - 1);
                frontUpMiddle = filter.getDouble("frontUp", filter.animateIn);
                if (filter.animateIn > 0)
                    frontUpStart = filter.getDouble("frontUp", 0);
                if (filter.animateOut > 0)
                    frontUpEnd = filter.getDouble("frontUp", filter.duration - 1);
                backXMiddle = filter.getDouble("backX", filter.animateIn);
                if (filter.animateIn > 0)
                    backXStart = filter.getDouble("backX", 0);
                if (filter.animateOut > 0)
                    backXEnd = filter.getDouble("backX", filter.duration - 1);
                backYMiddle = filter.getDouble("backY", filter.animateIn);
                if (filter.animateIn > 0)
                    backYStart = filter.getDouble("backY", 0);
                if (filter.animateOut > 0)
                    backYEnd = filter.getDouble("backY", filter.duration - 1);
                backUpMiddle = filter.getDouble("backUp", filter.animateIn);
                if (filter.animateIn > 0)
                    backUpStart = filter.getDouble("backUp", 0);
                if (filter.animateOut > 0)
                    backUpEnd = filter.getDouble("backUp", filter.duration - 1);
                fovMiddle = filter.getDouble("fov", filter.animateIn);
                if (filter.animateIn > 0)
                    fovStart = filter.getDouble("fov", 0);
                if (filter.animateOut > 0)
                    fovEnd = filter.getDouble("fov", filter.duration - 1);
                radiusMiddle = filter.getDouble("radius", filter.animateIn);
                if (filter.animateIn > 0)
                    radiusStart = filter.getDouble("radius", 0);
                if (filter.animateOut > 0)
                    radiusEnd = filter.getDouble("radius", filter.duration - 1);
                nadirRadiusMiddle = filter.getDouble("nadirRadius", filter.animateIn);
                if (filter.animateIn > 0)
                    nadirRadiusStart = filter.getDouble("nadirRadius", 0);
                if (filter.animateOut > 0)
                    nadirRadiusEnd = filter.getDouble("nadirRadius", filter.duration - 1);
                nadirCorrectionStartMiddle = filter.getDouble("nadirCorrectionStart", filter.animateIn);
                if (filter.animateIn > 0)
                    nadirCorrectionStartStart = filter.getDouble("nadirCorrectionStart", 0);
                if (filter.animateOut > 0)
                    nadirCorrectionStartEnd = filter.getDouble("nadirCorrectionStart", filter.duration - 1);
                interpolationValue = filter.get("interpolation");
                projectionValue = filter.get("projection");
                setControls(null);
            }
        }

        Label {
            text: qsTr('Interpolation')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: interpolationComboBox

            currentIndex: 0
            model: ["Nearest-neighbor", "Bilinear"]
            onCurrentIndexChanged: updateProperty_interpolation()
        }

        Shotcut.UndoButton {
            id: interpolationUndo

            onClicked: interpolationComboBox.currentIndex = 0
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: qsTr('Alignment')
            Layout.alignment: Qt.AlignLeft
            Layout.columnSpan: 4
        }

        Label {
            text: qsTr('Yaw')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: yawSlider

            minimumValue: -360
            maximumValue: 360
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_yaw(getPosition())
        }

        Shotcut.UndoButton {
            id: yawUndo

            onClicked: yawSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: yawKeyframesButton

            onToggled: {
                var value = yawSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("yaw");
                        yawSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("yaw");
                    blockUpdate = false;
                    filter.set("yaw", value, getPosition());
                } else {
                    filter.resetProperty("yaw");
                    filter.set("yaw", value);
                }
            }
        }

        Label {
            text: qsTr('Pitch', 'rotation around the side-to-side axis (roll, pitch, yaw)')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: pitchSlider

            minimumValue: -180
            maximumValue: 180
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_pitch(getPosition())
        }

        Shotcut.UndoButton {
            id: pitchUndo

            onClicked: pitchSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: pitchKeyframesButton

            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("pitch") > 0
            onToggled: {
                var value = pitchSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("pitch");
                        pitchSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("pitch");
                    blockUpdate = false;
                    filter.set("pitch", value, getPosition());
                } else {
                    filter.resetProperty("pitch");
                    filter.set("pitch", value);
                }
            }
        }

        Label {
            text: qsTr('Roll')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: rollSlider

            minimumValue: -180
            maximumValue: 180
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_roll(getPosition())
        }

        Shotcut.UndoButton {
            id: rollUndo

            onClicked: rollSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: rollKeyframesButton

            onToggled: {
                var value = rollSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("roll");
                        rollSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("roll");
                    blockUpdate = false;
                    filter.set("roll", value, getPosition());
                } else {
                    filter.resetProperty("roll");
                    filter.set("roll", value);
                }
            }
        }

        Label {
            text: qsTr('Lens')
            Layout.alignment: Qt.AlignLeft
            Layout.columnSpan: 4
        }

        Label {
            text: qsTr('Projection')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: projectionComboBox

            currentIndex: 0
            model: ["Equidistant Fisheye"]
            onCurrentIndexChanged: updateProperty_projection()
        }

        Shotcut.UndoButton {
            id: projectionUndo

            onClicked: projectionComboBox.currentIndex = 0
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: qsTr('FOV')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: fovSlider

            minimumValue: 0
            maximumValue: 360
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 4
            stepSize: 0.0001
            onValueChanged: updateProperty_fov(getPosition())
        }

        Shotcut.UndoButton {
            id: fovUndo

            onClicked: fovSlider.value = 180
        }

        Shotcut.KeyframesButton {
            id: fovKeyframesButton

            onToggled: {
                var value = fovSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("fov");
                        fovSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("fov");
                    blockUpdate = false;
                    filter.set("fov", value, getPosition());
                } else {
                    filter.resetProperty("fov");
                    filter.set("fov", value);
                }
            }
        }

        Label {
            text: qsTr('Radius')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: radiusSlider

            minimumValue: 0
            maximumValue: 1
            suffix: ' '
            decimals: 4
            stepSize: 0.0001
            onValueChanged: updateProperty_radius(getPosition())
        }

        Shotcut.UndoButton {
            id: radiusUndo

            onClicked: radiusSlider.value = 0.25
        }

        Shotcut.KeyframesButton {
            id: radiusKeyframesButton

            onToggled: {
                var value = radiusSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("radius");
                        radiusSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("radius");
                    blockUpdate = false;
                    filter.set("radius", value, getPosition());
                } else {
                    filter.resetProperty("radius");
                    filter.set("radius", value);
                }
            }
        }

        Label {
            text: qsTr('Front')
            Layout.alignment: Qt.AlignLeft
            Layout.columnSpan: 4
        }

        Label {
            text: qsTr('X')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: frontXSlider

            minimumValue: 0
            maximumValue: 1
            stepSize: 0.0001
            decimals: 4
            suffix: ' '
            onValueChanged: updateProperty_frontX(getPosition())
        }

        Shotcut.UndoButton {
            id: frontXUndo

            onClicked: frontXSlider.value = 0.25
        }

        Shotcut.KeyframesButton {
            id: frontXKeyframesButton

            onToggled: {
                var value = frontXSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("frontX");
                        frontXSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("frontX");
                    blockUpdate = false;
                    filter.set("frontX", value, getPosition());
                } else {
                    filter.resetProperty("frontX");
                    filter.set("frontX", value);
                }
            }
        }

        Label {
            text: qsTr('Y')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: frontYSlider

            minimumValue: 0
            maximumValue: 1
            stepSize: 0.0001
            decimals: 4
            suffix: ' '
            onValueChanged: updateProperty_frontY(getPosition())
        }

        Shotcut.UndoButton {
            id: frontYUndo

            onClicked: frontYSlider.value = 0.25
        }

        Shotcut.KeyframesButton {
            id: frontYKeyframesButton

            onToggled: {
                var value = frontYSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("frontY");
                        frontYSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("frontY");
                    blockUpdate = false;
                    filter.set("frontY", value, getPosition());
                } else {
                    filter.resetProperty("frontY");
                    filter.set("frontY", value);
                }
            }
        }

        Label {
            text: qsTr('Up')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: frontUpSlider

            minimumValue: 0
            maximumValue: 360
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_frontUp(getPosition())
        }

        Shotcut.UndoButton {
            id: frontUpUndo

            onClicked: frontUpSlider.value = 90
        }

        Shotcut.KeyframesButton {
            id: frontUpKeyframesButton

            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("frontUp") > 0
            onToggled: {
                var value = frontUpSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("frontUp");
                        frontUpSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("frontUp");
                    blockUpdate = false;
                    filter.set("frontUp", value, getPosition());
                } else {
                    filter.resetProperty("frontUp");
                    filter.set("frontUp", value);
                }
            }
        }

        Label {
            text: qsTr('Back')
            Layout.alignment: Qt.AlignLeft
            Layout.columnSpan: 4
        }

        Label {
            text: qsTr('X')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: backXSlider

            minimumValue: 0
            maximumValue: 1
            stepSize: 0.0001
            decimals: 4
            suffix: ' '
            onValueChanged: updateProperty_backX(getPosition())
        }

        Shotcut.UndoButton {
            id: backXUndo

            onClicked: backXSlider.value = 0.25
        }

        Shotcut.KeyframesButton {
            id: backXKeyframesButton

            onToggled: {
                var value = backXSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("backX");
                        backXSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("backX");
                    blockUpdate = false;
                    filter.set("backX", value, getPosition());
                } else {
                    filter.resetProperty("backX");
                    filter.set("backX", value);
                }
            }
        }

        Label {
            text: qsTr('Y')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: backYSlider

            minimumValue: 0
            maximumValue: 1
            stepSize: 0.0001
            decimals: 4
            suffix: ' '
            onValueChanged: updateProperty_backY(getPosition())
        }

        Shotcut.UndoButton {
            id: backYUndo

            onClicked: backYSlider.value = 0.25
        }

        Shotcut.KeyframesButton {
            id: backYKeyframesButton

            onToggled: {
                var value = backYSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("backY");
                        backYSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("backY");
                    blockUpdate = false;
                    filter.set("backY", value, getPosition());
                } else {
                    filter.resetProperty("backY");
                    filter.set("backY", value);
                }
            }
        }

        Label {
            text: qsTr('Up')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: backUpSlider

            minimumValue: 0
            maximumValue: 360
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_backUp(getPosition())
        }

        Shotcut.UndoButton {
            id: backUpUndo

            onClicked: backUpSlider.value = 90
        }

        Shotcut.KeyframesButton {
            id: backUpKeyframesButton

            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("backUp") > 0
            onToggled: {
                var value = backUpSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("backUp");
                        backUpSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("backUp");
                    blockUpdate = false;
                    filter.set("backUp", value, getPosition());
                } else {
                    filter.resetProperty("backUp");
                    filter.set("backUp", value);
                }
            }
        }

        Label {
            text: qsTr('Nadir')
            Layout.alignment: Qt.AlignLeft
            Layout.columnSpan: 4
        }

        Label {
            text: qsTr('Radius')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: nadirRadiusSlider

            minimumValue: 0
            maximumValue: 1
            suffix: ' '
            decimals: 4
            stepSize: 0.0001
            onValueChanged: updateProperty_nadirRadius(getPosition())
        }

        Shotcut.UndoButton {
            id: nadirRadiusUndo

            onClicked: nadirRadiusSlider.value = 0.2229
        }

        Shotcut.KeyframesButton {
            id: nadirRadiusKeyframesButton

            onToggled: {
                var value = nadirRadiusSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("nadirRadius");
                        nadirRadiusSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("nadirRadius");
                    blockUpdate = false;
                    filter.set("nadirRadius", value, getPosition());
                } else {
                    filter.resetProperty("nadirRadius");
                    filter.set("nadirRadius", value);
                }
            }
        }

        Label {
            text: qsTr('Start')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: nadirCorrectionStartSlider

            minimumValue: 0
            maximumValue: 1
            suffix: ' '
            decimals: 4
            stepSize: 0.0001
            onValueChanged: updateProperty_nadirCorrectionStart(getPosition())
        }

        Shotcut.UndoButton {
            id: nadirCorrectionStartUndo

            onClicked: radiusSlider.value = 0.8
        }

        Shotcut.KeyframesButton {
            id: nadirCorrectionStartKeyframesButton

            onToggled: {
                var value = nadirCorrectionStartSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("nadirCorrectionStart");
                        nadirCorrectionStartSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("nadirCorrectionStart");
                    blockUpdate = false;
                    filter.set("nadirCorrectionStart", value, getPosition());
                } else {
                    filter.resetProperty("nadirCorrectionStart");
                    filter.set("nadirCorrectionStart", value);
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        function onChanged() {
            setControls();
        }

        function onInChanged() {
            updateSimpleKeyframes();
        }

        function onOutChanged() {
            updateSimpleKeyframes();
        }

        function onAnimateInChanged() {
            updateSimpleKeyframes();
        }

        function onAnimateOutChanged() {
            updateSimpleKeyframes();
        }

        function onPropertyChanged(name) {
            setControls();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            setControls();
        }

        target: producer
    }
}
