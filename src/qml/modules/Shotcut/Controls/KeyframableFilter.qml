/*
 * Copyright (c) 2019-2020 Meltytech, LLC
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
import QtQuick 2.5

Item {
    // NOTE: This item only works for numeric parameters.

    // The following 4 arrays should be the same length and aligned to the
    // same parameters added to keyframableParameters. They should be set
    // by the filter that inherits this item.
    property var keyframableParameters: []
    // strings of MLT property names
    // The following 3 arrays are for simple keyframes.
    property var startValues: []
    property var middleValues: []
    property var endValues: []
    // Set this property true to guard against updateFilter() getting
    // called when setting a control value in code. Do not forget to reset it
    // to false when you are done setting controls' values.
    property bool blockUpdate: true

    // This function should be called in your Preset.beforePresetSelected handler.
    function resetSimpleKeyframes() {
        for (var i in keyframableParameters)
            filter.resetProperty(keyframableParameters[i]);
    }

    // This function should be called in your Preset.presetSelected handler.
    function initializeSimpleKeyframes() {
        for (var i in keyframableParameters) {
            var parameter = keyframableParameters[i];
            middleValues[i] = filter.getDouble(parameter, filter.animateIn);
            if (filter.animateIn > 0)
                startValues[i] = filter.getDouble(parameter, 0);
            if (filter.animateOut > 0)
                endValues[i] = filter.getDouble(parameter, filter.duration - 1);
        }
    }

    // This function should be called when calling updateFilter() (except
    // in filter Connections).
    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    // This function can be called in your setControls() function to determine
    // whether to enable keyframable controls.
    function isSimpleKeyframesActive() {
        var position = getPosition();
        return position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    // This function should be called in your controls' valueChanged handler.
    function updateFilter(parameter, value, button, position) {
        if (blockUpdate)
            return;
        var index = keyframableParameters.indexOf(parameter);
        if (index < 0) {
            console.log("ERROR: Parameter not found in keyframableParameters:", parameter);
            return;
        }
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValues[index] = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValues[index] = value;
            else
                middleValues[index] = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(parameter);
            button.checked = false;
            if (filter.animateIn > 0) {
                filter.set(parameter, startValues[index], 0);
                filter.set(parameter, middleValues[index], filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set(parameter, middleValues[index], filter.duration - filter.animateOut);
                filter.set(parameter, endValues[index], filter.duration - 1);
            }
        } else if (!button.checked) {
            filter.resetProperty(parameter);
            filter.set(parameter, middleValues[index]);
        } else if (position !== null) {
            filter.set(parameter, value, position);
        }
    }

    // This function should be called in the KeyframesButton.toggled handler.
    function toggleKeyframes(isEnabled, parameter, value) {
        if (isEnabled) {
            blockUpdate = true;
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                // Reset all of the simple keyframes.
                resetSimpleKeyframes();
                filter.animateIn = 0;
                blockUpdate = false;
                filter.animateOut = 0;
            } else {
                filter.clearSimpleAnimation(parameter);
                blockUpdate = false;
            }
            // Set this keyframe value.
            filter.set(parameter, value, getPosition());
        } else {
            // Remove keyframes and set the parameter.
            filter.resetProperty(parameter);
            filter.set(parameter, value);
        }
    }

    Component.onCompleted: {
        if (!filter.isNew)
            initializeSimpleKeyframes();
    }
}
