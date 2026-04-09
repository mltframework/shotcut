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
import Shotcut.Controls as Shotcut

Shotcut.KeyframableFilter {
    // Keyframable parameter identifiers
    property string sharpeningParam: '1'
    property string compNoiseIntParam: '4'
    property string snowParam: '6'
    property string headSwitchShiftParam: '14'
    property string trackingWaveParam: '17'
    property string ringingScaleParam: '22'

    // Defaults for the six keyframable parameters
    property double sharpeningDefault: 1.0
    property double compNoiseIntDefault: 0.05
    property double snowDefault: 0.00025
    property double headSwitchShiftDefault: 72.0
    property double trackingWaveDefault: 15.0
    property double ringingScaleDefault: 4.0

    function setControls() {
        const position = getPosition();
        blockUpdate = true;

        // General
        randomSeedSlider.value = filter.getDouble('36', position);
        useFieldCombo.currentIndex = Math.max(0, useFieldCombo.indexOfValue(filter.get('30')));
        lowpassTypeCombo.currentIndex = Math.max(0, lowpassTypeCombo.indexOfValue(filter.get('46')));
        inputLumaCombo.currentIndex = Math.max(0, inputLumaCombo.indexOfValue(filter.get('38')));
        chromaLowPassInCombo.currentIndex = Math.max(0, chromaLowPassInCombo.indexOfValue(filter.get('0')));
        sharpeningSlider.value = filter.getDouble(sharpeningParam, position);
        snowSlider.value = filter.getDouble(snowParam, position);
        snowAnisotropySlider.value = filter.getDouble('34', position);
        scanlinePhaseShiftCombo.currentIndex = Math.max(0, scanlinePhaseShiftCombo.indexOfValue(filter.get('2')));
        scanlinePhaseOffsetSlider.value = filter.getDouble('3', position);
        chromaDemodCombo.currentIndex = Math.max(0, chromaDemodCombo.indexOfValue(filter.get('33')));
        lumaSmearSlider.value = filter.getDouble('45', position);
        vertBlendChromaCheck.checked = filter.get('25') === '1';
        chromaLowPassOutCombo.currentIndex = Math.max(0, chromaLowPassOutCombo.indexOfValue(filter.get('10')));
        chromaPhaseErrorSlider.value = filter.getDouble('37', position);
        chromaPhaseNoiseSlider.value = filter.getDouble('7', position);
        chromaDelayHSlider.value = filter.getDouble('8', position);
        chromaDelayVSlider.value = filter.getDouble('9', position);
        srgbGammaCheck.checked = filter.get('SrgbGammaCorrect') === '1';

        // Keyframes button states
        sharpeningKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(sharpeningParam) > 0;
        compNoiseIntKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(compNoiseIntParam) > 0;
        snowKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(snowParam) > 0;
        headSwitchShiftKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(headSwitchShiftParam) > 0;
        trackingWaveKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(trackingWaveParam) > 0;
        ringingScaleKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(ringingScaleParam) > 0;

        // Composite signal noise
        compNoiseEnabledCheck.checked = filter.get('52') === '1';
        compNoiseIntSlider.value = filter.getDouble(compNoiseIntParam, position);
        compNoiseFreqSlider.value = filter.getDouble('53', position);
        compNoiseDetailSlider.value = filter.getDouble('54', position);

        // Head switching
        headSwitchEnabledCheck.checked = filter.get('11') === '1';
        headSwitchHeightSlider.value = filter.getDouble('12', position);
        headSwitchOffsetSlider.value = filter.getDouble('13', position);
        headSwitchShiftSlider.value = filter.getDouble(headSwitchShiftParam, position);
        startMidLineEnabledCheck.checked = filter.get('49') === '1';
        startMidLinePosSlider.value = filter.getDouble('50', position);
        startMidLineJitterSlider.value = filter.getDouble('51', position);

        // Tracking noise
        trackingNoiseEnabledCheck.checked = filter.get('15') === '1';
        trackingNoiseHeightSlider.value = filter.getDouble('16', position);
        trackingWaveSlider.value = filter.getDouble(trackingWaveParam, position);
        trackingSnowIntSlider.value = filter.getDouble('18', position);
        trackingSnowAnisoSlider.value = filter.getDouble('35', position);
        trackingNoiseIntSlider.value = filter.getDouble('31', position);

        // Ringing
        ringingEnabledCheck.checked = filter.get('19') === '1';
        ringingFreqSlider.value = filter.getDouble('20', position);
        ringingPowerSlider.value = filter.getDouble('21', position);
        ringingScaleSlider.value = filter.getDouble(ringingScaleParam, position);

        // Luma noise
        lumaNoiseEnabledCheck.checked = filter.get('55') === '1';
        lumaNoiseIntSlider.value = filter.getDouble('57', position);
        lumaNoiseFreqSlider.value = filter.getDouble('56', position);
        lumaNoiseDetailSlider.value = filter.getDouble('58', position);

        // Chroma noise
        chromaNoiseEnabledCheck.checked = filter.get('42') === '1';
        chromaNoiseIntSlider.value = filter.getDouble('5', position);
        chromaNoiseFreqSlider.value = filter.getDouble('43', position);
        chromaNoiseDetailSlider.value = filter.getDouble('44', position);

        // VHS emulation
        vhsEnabledCheck.checked = filter.get('23') === '1';
        vhsTapeSpeedCombo.currentIndex = Math.max(0, vhsTapeSpeedCombo.indexOfValue(filter.get('24')));
        vhsChromaLossSlider.value = filter.getDouble('26', position);
        vhsSharpenEnabledCheck.checked = filter.get('47') === '1';
        vhsSharpenIntSlider.value = filter.getDouble('27', position);
        vhsSharpenFreqSlider.value = filter.getDouble('48', position);
        edgeWaveEnabledCheck.checked = filter.get('39') === '1';
        edgeWaveIntSlider.value = filter.getDouble('28', position);
        edgeWaveSpeedSlider.value = filter.getDouble('29', position);
        edgeWaveFreqSlider.value = filter.getDouble('40', position);
        edgeWaveDetailSlider.value = filter.getDouble('41', position);

        // Scale
        scaleEnabledCheck.checked = filter.get('61') === '1';
        horizScaleSlider.value = filter.getDouble('32', position);
        vertScaleSlider.value = filter.getDouble('59', position);
        scaleWithVideoSizeCheck.checked = filter.get('60') === '1';

        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        sharpeningSlider.enabled = enabled;
        compNoiseIntSlider.enabled = enabled;
        snowSlider.enabled = enabled;
        headSwitchShiftSlider.enabled = enabled;
        trackingWaveSlider.enabled = enabled;
        ringingScaleSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        setControls();
        updateFilter(sharpeningParam, sharpeningSlider.value, sharpeningKeyframesButton, null);
        updateFilter(compNoiseIntParam, compNoiseIntSlider.value, compNoiseIntKeyframesButton, null);
        updateFilter(snowParam, snowSlider.value, snowKeyframesButton, null);
        updateFilter(headSwitchShiftParam, headSwitchShiftSlider.value, headSwitchShiftKeyframesButton, null);
        updateFilter(trackingWaveParam, trackingWaveSlider.value, trackingWaveKeyframesButton, null);
        updateFilter(ringingScaleParam, ringingScaleSlider.value, ringingScaleKeyframesButton, null);
    }

    keyframableParameters: [sharpeningParam, compNoiseIntParam, snowParam, headSwitchShiftParam, trackingWaveParam, ringingScaleParam]
    startValues: [1.0, 0.05, 0.00025, 72.0, 15.0, 4.0]
    middleValues: [1.0, 0.05, 0.00025, 72.0, 15.0, 4.0]
    endValues: [1.0, 0.05, 0.00025, 72.0, 15.0, 4.0]
    width: 350
    height: 2100

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('36', 0);
            filter.set('30', 'Interleaved (upper first)');
            filter.set('46', 'Butterworth (sharper)');
            filter.set('38', 'Notch');
            filter.set('0', 'Full');
            filter.set(sharpeningParam, sharpeningDefault);
            filter.set('52', 1);
            filter.set(compNoiseIntParam, compNoiseIntDefault);
            filter.set('53', 0.5);
            filter.set('54', 1);
            filter.set(snowParam, snowDefault);
            filter.set('34', 0.5);
            filter.set('2', '180 degrees');
            filter.set('3', 0);
            filter.set('33', 'Notch');
            filter.set('45', 0.5);
            filter.set('11', 1);
            filter.set('12', 8);
            filter.set('13', 3);
            filter.set(headSwitchShiftParam, headSwitchShiftDefault);
            filter.set('49', 1);
            filter.set('50', 0.95);
            filter.set('51', 0.03);
            filter.set('15', 1);
            filter.set('16', 12);
            filter.set(trackingWaveParam, trackingWaveDefault);
            filter.set('18', 0.025);
            filter.set('35', 0.25);
            filter.set('31', 0.25);
            filter.set('19', 1);
            filter.set('20', 0.45);
            filter.set('21', 4.0);
            filter.set(ringingScaleParam, ringingScaleDefault);
            filter.set('55', 1);
            filter.set('57', 0.01);
            filter.set('56', 0.5);
            filter.set('58', 1);
            filter.set('42', 1);
            filter.set('5', 0.1);
            filter.set('43', 0.05);
            filter.set('44', 2);
            filter.set('37', 0.0);
            filter.set('7', 0.001);
            filter.set('8', 0.0);
            filter.set('9', 0);
            filter.set('23', 1);
            filter.set('24', 'LP (Long Play)');
            filter.set('26', 2.5e-05);
            filter.set('47', 1);
            filter.set('27', 0.25);
            filter.set('48', 1.0);
            filter.set('39', 1);
            filter.set('28', 0.5);
            filter.set('29', 4.0);
            filter.set('40', 0.05);
            filter.set('41', 2);
            filter.set('25', 1);
            filter.set('10', 'Full');
            filter.set('61', 1);
            filter.set('32', 1.0);
            filter.set('59', 1.0);
            filter.set('60', 0);
            filter.set('SrgbGammaCorrect', 0);
            filter.savePreset(preset.parameters);
        }
        setControls();
    }

    SystemPalette {
        id: activePalette
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 4

        // Preset row
        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.Preset {
            id: preset
            parameters: ['36', '30', '46', '38', '0', '1', '52', '4', '53', '54',
                         '6', '34', '2', '3', '33', '45', '11', '12', '13', '14',
                         '49', '50', '51', '15', '16', '17', '18', '35', '31',
                         '19', '20', '21', '22', '55', '57', '56', '58', '42',
                         '5', '43', '44', '37', '7', '8', '9', '23', '24', '26',
                         '47', '27', '48', '39', '28', '29', '40', '41', '25',
                         '10', '61', '32', '59', '60', 'SrgbGammaCorrect']
            Layout.columnSpan: 3
            onBeforePresetLoaded: resetSimpleKeyframes()
            onPresetSelected: {
                setControls();
                initializeSimpleKeyframes();
            }
        }

        // ── General ──────────────────────────────────────────────────────────
        RowLayout {
            Layout.columnSpan: 4

            Label {
                text: qsTr('General')
                font.bold: true
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: activePalette.text
                opacity: 0.4
            }
        }

        Label {
            text: qsTr('Random seed')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: randomSeedSlider
            minimumValue: 0
            maximumValue: 9999
            decimals: 0
            stepSize: 1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('36', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: randomSeedSlider.value = 0
        }
        Item {}

        Label {
            text: qsTr('Use field')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            id: useFieldCombo
            implicitWidth: 200
            textRole: 'text'
            valueRole: 'value'
            model: [
                {value: 'Alternating', text: qsTr('Alternating')},
                {value: 'Upper only', text: qsTr('Upper only')},
                {value: 'Lower only', text: qsTr('Lower only')},
                {value: 'Interleaved (upper first)', text: qsTr('Interleaved (upper first)')},
                {value: 'Interleaved (lower first)', text: qsTr('Interleaved (lower first)')},
                {value: 'Both', text: qsTr('Both')}
            ]
            onActivated: {
                if (blockUpdate)
                    return;
                filter.set('30', currentValue);
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                useFieldCombo.currentIndex = useFieldCombo.indexOfValue('Interleaved (upper first)');
                filter.set('30', useFieldCombo.currentValue);
            }
        }
        Item {}

        Label {
            text: qsTr('Lowpass filter type')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            id: lowpassTypeCombo
            implicitWidth: 200
            textRole: 'text'
            valueRole: 'value'
            model: [
                {value: 'Constant K (blurry)', text: qsTr('Constant K (blurry)')},
                {value: 'Butterworth (sharper)', text: qsTr('Butterworth (sharper)')}
            ]
            onActivated: {
                if (blockUpdate)
                    return;
                filter.set('46', currentValue);
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                lowpassTypeCombo.currentIndex = lowpassTypeCombo.indexOfValue('Butterworth (sharper)');
                filter.set('46', lowpassTypeCombo.currentValue);
            }
        }
        Item {}

        Label {
            text: qsTr('Input luma filter')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            id: inputLumaCombo
            implicitWidth: 200
            textRole: 'text'
            valueRole: 'value'
            model: [
                {value: 'Notch', text: qsTr('Notch')},
                {value: 'Box', text: qsTr('Box')},
                {value: 'None', text: qsTr('None')}
            ]
            onActivated: {
                if (blockUpdate)
                    return;
                filter.set('38', currentValue);
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                inputLumaCombo.currentIndex = inputLumaCombo.indexOfValue('Notch');
                filter.set('38', inputLumaCombo.currentValue);
            }
        }
        Item {}

        Label {
            text: qsTr('Chroma low-pass in')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            id: chromaLowPassInCombo
            implicitWidth: 200
            textRole: 'text'
            valueRole: 'value'
            model: [
                {value: 'Full', text: qsTr('Full')},
                {value: 'Light', text: qsTr('Light')},
                {value: 'None', text: qsTr('None')}
            ]
            onActivated: {
                if (blockUpdate)
                    return;
                filter.set('0', currentValue);
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                chromaLowPassInCombo.currentIndex = chromaLowPassInCombo.indexOfValue('Full');
                filter.set('0', chromaLowPassInCombo.currentValue);
            }
        }
        Item {}

        Label {
            text: qsTr('Signal sharpening')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: sharpeningSlider
            minimumValue: -1
            maximumValue: 2
            decimals: 2
            stepSize: 0.01
            onValueChanged: updateFilter(sharpeningParam, value, sharpeningKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: sharpeningSlider.value = sharpeningDefault
        }
        Shotcut.KeyframesButton {
            id: sharpeningKeyframesButton
            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, sharpeningParam, sharpeningSlider.value);
            }
        }

        Label {
            text: qsTr('Snow')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: snowSlider
            minimumValue: 0
            maximumValue: 100
            decimals: 5
            stepSize: 0.00001
            onValueChanged: updateFilter(snowParam, value, snowKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: snowSlider.value = snowDefault
        }
        Shotcut.KeyframesButton {
            id: snowKeyframesButton
            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, snowParam, snowSlider.value);
            }
        }

        Label {
            text: qsTr('Snow anisotropy')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: snowAnisotropySlider
            minimumValue: 0
            maximumValue: 1
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('34', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: snowAnisotropySlider.value = 0.5
        }
        Item {}

        Label {
            text: qsTr('Scanline phase shift')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            id: scanlinePhaseShiftCombo
            implicitWidth: 200
            textRole: 'text'
            valueRole: 'value'
            model: [
                {value: '0 degrees', text: qsTr('0 degrees')},
                {value: '90 degrees', text: qsTr('90 degrees')},
                {value: '180 degrees', text: qsTr('180 degrees')},
                {value: '270 degrees', text: qsTr('270 degrees')}
            ]
            onActivated: {
                if (blockUpdate)
                    return;
                filter.set('2', currentValue);
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                scanlinePhaseShiftCombo.currentIndex = scanlinePhaseShiftCombo.indexOfValue('180 degrees');
                filter.set('2', scanlinePhaseShiftCombo.currentValue);
            }
        }
        Item {}

        Label {
            text: qsTr('Phase shift offset')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: scanlinePhaseOffsetSlider
            minimumValue: 0
            maximumValue: 3
            decimals: 0
            stepSize: 1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('3', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: scanlinePhaseOffsetSlider.value = 0
        }
        Item {}

        Label {
            text: qsTr('Chroma demod filter')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            id: chromaDemodCombo
            implicitWidth: 200
            textRole: 'text'
            valueRole: 'value'
            model: [
                {value: 'Box', text: qsTr('Box')},
                {value: 'Notch', text: qsTr('Notch')},
                {value: '1-line comb', text: qsTr('1-line comb')},
                {value: '2-line comb', text: qsTr('2-line comb')}
            ]
            onActivated: {
                if (blockUpdate)
                    return;
                filter.set('33', currentValue);
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                chromaDemodCombo.currentIndex = chromaDemodCombo.indexOfValue('Notch');
                filter.set('33', chromaDemodCombo.currentValue);
            }
        }
        Item {}

        Label {
            text: qsTr('Luma smear')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: lumaSmearSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('45', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: lumaSmearSlider.value = 0.5
        }
        Item {}

        Label {
            text: qsTr('Chroma low-pass out')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            id: chromaLowPassOutCombo
            implicitWidth: 200
            textRole: 'text'
            valueRole: 'value'
            model: [
                {value: 'Full', text: qsTr('Full')},
                {value: 'Light', text: qsTr('Light')},
                {value: 'None', text: qsTr('None')}
            ]
            onActivated: {
                if (blockUpdate)
                    return;
                filter.set('10', currentValue);
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                chromaLowPassOutCombo.currentIndex = chromaLowPassOutCombo.indexOfValue('Full');
                filter.set('10', chromaLowPassOutCombo.currentValue);
            }
        }
        Item {}

        Label {
            text: qsTr('Chroma phase error')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: chromaPhaseErrorSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('37', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: chromaPhaseErrorSlider.value = 0
        }
        Item {}

        Label {
            text: qsTr('Chroma phase noise')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: chromaPhaseNoiseSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            stepSize: 0.001
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('7', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: chromaPhaseNoiseSlider.value = 0.001
        }
        Item {}

        Label {
            text: qsTr('Chroma delay (H)')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: chromaDelayHSlider
            minimumValue: -40
            maximumValue: 40
            decimals: 1
            stepSize: 0.1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('8', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: chromaDelayHSlider.value = 0
        }
        Item {}

        Label {
            text: qsTr('Chroma delay (V)')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: chromaDelayVSlider
            minimumValue: -20
            maximumValue: 20
            decimals: 0
            stepSize: 1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('9', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: chromaDelayVSlider.value = 0
        }
        Item {}

        Item {}
        CheckBox {
            id: vertBlendChromaCheck
            text: qsTr('Vertically blend chroma')
            onToggled: {
                if (blockUpdate)
                    return;
                filter.set('25', checked ? 1 : 0);
            }
        }
        Item {}
        Item {}

        Item {}
        CheckBox {
            id: srgbGammaCheck
            text: qsTr('Apply sRGB gamma')
            onToggled: {
                if (blockUpdate)
                    return;
                filter.set('SrgbGammaCorrect', checked ? 1 : 0);
            }
        }
        Item {}
        Item {}

        // ── Composite Signal Noise ────────────────────────────────────────────
        RowLayout {
            Layout.columnSpan: 4

            CheckBox {
                id: compNoiseEnabledCheck
                text: qsTr('Composite Signal Noise')
                font.bold: true
                onToggled: {
                    if (blockUpdate)
                        return;
                    filter.set('52', checked ? 1 : 0);
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: activePalette.text
                opacity: 0.4
            }
        }

        Label {
            text: qsTr('Intensity')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: compNoiseIntSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            stepSize: 0.001
            onValueChanged: updateFilter(compNoiseIntParam, value, compNoiseIntKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: compNoiseIntSlider.value = compNoiseIntDefault
        }
        Shotcut.KeyframesButton {
            id: compNoiseIntKeyframesButton
            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, compNoiseIntParam, compNoiseIntSlider.value);
            }
        }

        Label {
            text: qsTr('Frequency')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: compNoiseFreqSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('53', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: compNoiseFreqSlider.value = 0.5
        }
        Item {}

        Label {
            text: qsTr('Detail')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: compNoiseDetailSlider
            minimumValue: 1
            maximumValue: 5
            decimals: 0
            stepSize: 1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('54', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: compNoiseDetailSlider.value = 1
        }
        Item {}

        // ── Head Switching ────────────────────────────────────────────────────
        RowLayout {
            Layout.columnSpan: 4

            CheckBox {
                id: headSwitchEnabledCheck
                text: qsTr('Head Switching')
                font.bold: true
                onToggled: {
                    if (blockUpdate)
                        return;
                    filter.set('11', checked ? 1 : 0);
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: activePalette.text
                opacity: 0.4
            }
        }

        Label {
            text: qsTr('Height')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: headSwitchHeightSlider
            minimumValue: 0
            maximumValue: 24
            decimals: 0
            stepSize: 1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('12', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: headSwitchHeightSlider.value = 8
        }
        Item {}

        Label {
            text: qsTr('Offset')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: headSwitchOffsetSlider
            minimumValue: 0
            maximumValue: 24
            decimals: 0
            stepSize: 1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('13', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: headSwitchOffsetSlider.value = 3
        }
        Item {}

        Label {
            text: qsTr('Horizontal shift')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: headSwitchShiftSlider
            minimumValue: -100
            maximumValue: 100
            decimals: 1
            stepSize: 0.1
            onValueChanged: updateFilter(headSwitchShiftParam, value, headSwitchShiftKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: headSwitchShiftSlider.value = headSwitchShiftDefault
        }
        Shotcut.KeyframesButton {
            id: headSwitchShiftKeyframesButton
            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, headSwitchShiftParam, headSwitchShiftSlider.value);
            }
        }

        RowLayout {
            Layout.columnSpan: 4

            CheckBox {
                id: startMidLineEnabledCheck
                leftPadding: 20
                text: qsTr('Start mid-line')
                onToggled: {
                    if (blockUpdate)
                        return;
                    filter.set('49', checked ? 1 : 0);
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: activePalette.text
                opacity: 0.4
            }
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: startMidLinePosSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('50', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: startMidLinePosSlider.value = 0.95
        }
        Item {}

        Label {
            text: qsTr('Jitter')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: startMidLineJitterSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('51', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: startMidLineJitterSlider.value = 0.03
        }
        Item {}

        // ── Tracking Noise ────────────────────────────────────────────────────
        RowLayout {
            Layout.columnSpan: 4

            CheckBox {
                id: trackingNoiseEnabledCheck
                text: qsTr('Tracking Noise')
                font.bold: true
                onToggled: {
                    if (blockUpdate)
                        return;
                    filter.set('15', checked ? 1 : 0);
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: activePalette.text
                opacity: 0.4
            }
        }

        Label {
            text: qsTr('Height')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: trackingNoiseHeightSlider
            minimumValue: 0
            maximumValue: 120
            decimals: 0
            stepSize: 1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('16', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: trackingNoiseHeightSlider.value = 12
        }
        Item {}

        Label {
            text: qsTr('Wave intensity')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: trackingWaveSlider
            minimumValue: -50
            maximumValue: 50
            decimals: 1
            stepSize: 0.1
            onValueChanged: updateFilter(trackingWaveParam, value, trackingWaveKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: trackingWaveSlider.value = trackingWaveDefault
        }
        Shotcut.KeyframesButton {
            id: trackingWaveKeyframesButton
            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, trackingWaveParam, trackingWaveSlider.value);
            }
        }

        Label {
            text: qsTr('Snow intensity')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: trackingSnowIntSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            stepSize: 0.001
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('18', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: trackingSnowIntSlider.value = 0.025
        }
        Item {}

        Label {
            text: qsTr('Snow anisotropy')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: trackingSnowAnisoSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('35', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: trackingSnowAnisoSlider.value = 0.25
        }
        Item {}

        Label {
            text: qsTr('Noise intensity')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: trackingNoiseIntSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('31', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: trackingNoiseIntSlider.value = 0.25
        }
        Item {}

        // ── Ringing ───────────────────────────────────────────────────────────
        RowLayout {
            Layout.columnSpan: 4

            CheckBox {
                id: ringingEnabledCheck
                text: qsTr('Ringing')
                font.bold: true
                onToggled: {
                    if (blockUpdate)
                        return;
                    filter.set('19', checked ? 1 : 0);
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: activePalette.text
                opacity: 0.4
            }
        }

        Label {
            text: qsTr('Frequency')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: ringingFreqSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('20', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: ringingFreqSlider.value = 0.45
        }
        Item {}

        Label {
            text: qsTr('Power')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: ringingPowerSlider
            minimumValue: 1
            maximumValue: 10
            decimals: 1
            stepSize: 0.1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('21', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: ringingPowerSlider.value = 4.0
        }
        Item {}

        Label {
            text: qsTr('Scale')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: ringingScaleSlider
            minimumValue: 0
            maximumValue: 10
            decimals: 1
            stepSize: 0.1
            onValueChanged: updateFilter(ringingScaleParam, value, ringingScaleKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: ringingScaleSlider.value = ringingScaleDefault
        }
        Shotcut.KeyframesButton {
            id: ringingScaleKeyframesButton
            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, ringingScaleParam, ringingScaleSlider.value);
            }
        }

        // ── Luma Noise ────────────────────────────────────────────────────────
        RowLayout {
            Layout.columnSpan: 4

            CheckBox {
                id: lumaNoiseEnabledCheck
                text: qsTr('Luma Noise')
                font.bold: true
                onToggled: {
                    if (blockUpdate)
                        return;
                    filter.set('55', checked ? 1 : 0);
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: activePalette.text
                opacity: 0.4
            }
        }

        Label {
            text: qsTr('Intensity')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: lumaNoiseIntSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            stepSize: 0.001
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('57', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: lumaNoiseIntSlider.value = 0.01
        }
        Item {}

        Label {
            text: qsTr('Frequency')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: lumaNoiseFreqSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('56', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: lumaNoiseFreqSlider.value = 0.5
        }
        Item {}

        Label {
            text: qsTr('Detail')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: lumaNoiseDetailSlider
            minimumValue: 1
            maximumValue: 5
            decimals: 0
            stepSize: 1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('58', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: lumaNoiseDetailSlider.value = 1
        }
        Item {}

        // ── Chroma Noise ──────────────────────────────────────────────────────
        RowLayout {
            Layout.columnSpan: 4

            CheckBox {
                id: chromaNoiseEnabledCheck
                text: qsTr('Chroma Noise')
                font.bold: true
                onToggled: {
                    if (blockUpdate)
                        return;
                    filter.set('42', checked ? 1 : 0);
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: activePalette.text
                opacity: 0.4
            }
        }

        Label {
            text: qsTr('Intensity')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: chromaNoiseIntSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('5', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: chromaNoiseIntSlider.value = 0.1
        }
        Item {}

        Label {
            text: qsTr('Frequency')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: chromaNoiseFreqSlider
            minimumValue: 0
            maximumValue: 0.5
            decimals: 3
            stepSize: 0.001
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('43', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: chromaNoiseFreqSlider.value = 0.05
        }
        Item {}

        Label {
            text: qsTr('Detail')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: chromaNoiseDetailSlider
            minimumValue: 1
            maximumValue: 5
            decimals: 0
            stepSize: 1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('44', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: chromaNoiseDetailSlider.value = 2
        }
        Item {}

        // ── VHS Emulation ─────────────────────────────────────────────────────
        RowLayout {
            Layout.columnSpan: 4

            CheckBox {
                id: vhsEnabledCheck
                text: qsTr('VHS Emulation')
                font.bold: true
                onToggled: {
                    if (blockUpdate)
                        return;
                    filter.set('23', checked ? 1 : 0);
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: activePalette.text
                opacity: 0.4
            }
        }

        Label {
            text: qsTr('Tape speed')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            id: vhsTapeSpeedCombo
            implicitWidth: 200
            textRole: 'text'
            valueRole: 'value'
            model: [
                {value: 'SP (Standard Play)', text: qsTr('SP (Standard Play)')},
                {value: 'LP (Long Play)', text: qsTr('LP (Long Play)')},
                {value: 'EP (Extended Play)', text: qsTr('EP (Extended Play)')},
                {value: 'None', text: qsTr('None')}
            ]
            onActivated: {
                if (blockUpdate)
                    return;
                filter.set('24', currentValue);
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                vhsTapeSpeedCombo.currentIndex = vhsTapeSpeedCombo.indexOfValue('LP (Long Play)');
                filter.set('24', vhsTapeSpeedCombo.currentValue);
            }
        }
        Item {}

        Label {
            text: qsTr('Chroma loss')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: vhsChromaLossSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 5
            stepSize: 0.00001
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('26', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: vhsChromaLossSlider.value = 2.5e-05
        }
        Item {}

        RowLayout {
            Layout.columnSpan: 4

            CheckBox {
                id: vhsSharpenEnabledCheck
                leftPadding: 20
                text: qsTr('Sharpen')
                onToggled: {
                    if (blockUpdate)
                        return;
                    filter.set('47', checked ? 1 : 0);
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: activePalette.text
                opacity: 0.4
            }
        }

        Label {
            text: qsTr('Intensity')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: vhsSharpenIntSlider
            minimumValue: 0
            maximumValue: 5
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('27', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: vhsSharpenIntSlider.value = 0.25
        }
        Item {}

        Label {
            text: qsTr('Frequency')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: vhsSharpenFreqSlider
            minimumValue: 0.5
            maximumValue: 4
            decimals: 2
            stepSize: 0.01
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('48', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: vhsSharpenFreqSlider.value = 1.0
        }
        Item {}

        RowLayout {
            Layout.columnSpan: 4

            CheckBox {
                id: edgeWaveEnabledCheck
                leftPadding: 20
                text: qsTr('Edge Wave')
                onToggled: {
                    if (blockUpdate)
                        return;
                    filter.set('39', checked ? 1 : 0);
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: activePalette.text
                opacity: 0.4
            }
        }

        Label {
            text: qsTr('Intensity')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: edgeWaveIntSlider
            minimumValue: 0
            maximumValue: 20
            decimals: 1
            stepSize: 0.1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('28', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: edgeWaveIntSlider.value = 0.5
        }
        Item {}

        Label {
            text: qsTr('Speed')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: edgeWaveSpeedSlider
            minimumValue: 0
            maximumValue: 10
            decimals: 1
            stepSize: 0.1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('29', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: edgeWaveSpeedSlider.value = 4.0
        }
        Item {}

        Label {
            text: qsTr('Frequency')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: edgeWaveFreqSlider
            minimumValue: 0
            maximumValue: 0.5
            decimals: 3
            stepSize: 0.001
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('40', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: edgeWaveFreqSlider.value = 0.05
        }
        Item {}

        Label {
            text: qsTr('Detail')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: edgeWaveDetailSlider
            minimumValue: 1
            maximumValue: 5
            decimals: 0
            stepSize: 1
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('41', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: edgeWaveDetailSlider.value = 2
        }
        Item {}

        // ── Scale ─────────────────────────────────────────────────────────────
        RowLayout {
            Layout.columnSpan: 4

            CheckBox {
                id: scaleEnabledCheck
                text: qsTr('Scale')
                font.bold: true
                onToggled: {
                    if (blockUpdate)
                        return;
                    filter.set('61', checked ? 1 : 0);
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: activePalette.text
                opacity: 0.4
            }
        }

        Label {
            text: qsTr('Horizontal scale')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: horizScaleSlider
            minimumValue: 0.125
            maximumValue: 8
            decimals: 3
            stepSize: 0.001
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('32', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: horizScaleSlider.value = 1.0
        }
        Item {}

        Label {
            text: qsTr('Vertical scale')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: vertScaleSlider
            minimumValue: 0.125
            maximumValue: 8.8
            decimals: 3
            stepSize: 0.001
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('59', value);
            }
        }
        Shotcut.UndoButton {
            onClicked: vertScaleSlider.value = 1.0
        }
        Item {}

        Item {}
        CheckBox {
            id: scaleWithVideoSizeCheck
            text: qsTr('Scale with video size')
            onToggled: {
                if (blockUpdate)
                    return;
                filter.set('60', checked ? 1 : 0);
            }
        }
        Item {}
        Item {}

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
