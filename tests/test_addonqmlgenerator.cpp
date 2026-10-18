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

#include "controllers/addonqmlgenerator.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class TestAddOnQmlGenerator : public QObject
{
    Q_OBJECT

private:
    static QString readFile(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return QString();
        return QString::fromUtf8(file.readAll());
    }

    static AddOnParameterDescriptor parameter(const QString &name,
                                              const QString &type,
                                              bool keyframes = false,
                                              const QString &widget = QString())
    {
        AddOnParameterDescriptor out;
        out.name = name;
        out.title = name;
        out.type = type;
        out.widget = widget;
        out.supportsKeyframes = keyframes;
        return out;
    }

    static bool generate(const AddOnFilterDescriptor &descriptor,
                         QTemporaryDir &dir,
                         QString *ui,
                         QString *vui,
                         QString *meta)
    {
        AddOnQmlGenerator generator;
        QString error;
        const QDir output(dir.path());
        if (!generator.generate(descriptor, output, QStringLiteral("ui.qml"), &error))
            return false;
        if (!generator.generateMetaQml(descriptor,
                                       output,
                                       QStringLiteral("ui.qml"),
                                       QStringLiteral("meta.qml"),
                                       &error))
            return false;
        if (ui)
            *ui = readFile(output.filePath(QStringLiteral("ui.qml")));
        if (vui)
            *vui = readFile(output.filePath(QStringLiteral("vui.qml")));
        if (meta)
            *meta = readFile(output.filePath(QStringLiteral("meta.qml")));
        return true;
    }

private slots:
    void pointWidgetHasNoRectangleVui()
    {
        AddOnFilterDescriptor descriptor;
        descriptor.service = QStringLiteral("test.point");
        descriptor.title = QStringLiteral("Point");
        descriptor.parameters << parameter(QStringLiteral("pos"),
                                           QStringLiteral("rect"),
                                           true,
                                           QStringLiteral("point"));

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString ui;
        QString meta;
        QVERIFY(generate(descriptor, dir, &ui, nullptr, &meta));
        QVERIFY(!QFileInfo::exists(dir.filePath(QStringLiteral("vui.qml"))));
        QVERIFY(ui.contains(QStringLiteral("Shotcut.Number2D")));
        QVERIFY(!meta.contains(QStringLiteral("vui:")));
        QVERIFY(!meta.contains(QStringLiteral("isRectangle")));
        QVERIFY(ui.contains(QStringLiteral("keyframableParameters: []")));
    }

    void bareRectIsRectangleVuiAndKeyframable()
    {
        AddOnFilterDescriptor descriptor;
        descriptor.service = QStringLiteral("test.rect");
        descriptor.title = QStringLiteral("Rect");
        descriptor.parameters << parameter(QStringLiteral("geometry"), QStringLiteral("rect"), true);
        descriptor.parameters << parameter(QStringLiteral("rotate_center"),
                                           QStringLiteral("integer"),
                                           false,
                                           QStringLiteral("checkbox"));
        descriptor.parameters << parameter(QStringLiteral("transition.rotation"),
                                           QStringLiteral("float"),
                                           true);
        descriptor.parameters << parameter(QStringLiteral("anchor"), QStringLiteral("rect"), true);
        descriptor.parameters.back().normalizedCoordinates = false;
        descriptor.parameters[0].normalizedCoordinates = true;

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString ui;
        QString vui;
        QString meta;
        QVERIFY(generate(descriptor, dir, &ui, &vui, &meta));
        QVERIFY(vui.contains(QStringLiteral("Shotcut.RectangleVui")));
        QVERIFY(vui.contains(QStringLiteral("rectProperty: 'geometry'")));
        QVERIFY(vui.contains(QStringLiteral("rotationProperty: 'transition.rotation'")));
        QVERIFY(vui.contains(QStringLiteral("rectKeyframes: true")));
        QVERIFY(vui.contains(QStringLiteral("rotationKeyframes: true")));
        QVERIFY(vui.contains(QStringLiteral("normalizedCoordinates: true")));
        QVERIFY(!vui.contains(QStringLiteral("rotate_center")));
        QVERIFY(!vui.contains(QStringLiteral("rectProperty: 'anchor'")));
        QVERIFY(meta.contains(QStringLiteral("vui: 'vui.qml'")));
        QVERIFY(meta.contains(QStringLiteral("property: 'geometry'")));
        QVERIFY(meta.contains(QStringLiteral("property: 'anchor'")));
        QVERIFY(meta.contains(QStringLiteral("property: 'transition.rotation'")));
        QVERIFY(meta.contains(QStringLiteral("isCurve: true")));
        QCOMPARE(meta.count(QStringLiteral("isRectangle: true")), 2);
        QVERIFY(
            meta.contains(QStringLiteral("simpleProperties: ['geometry', 'transition.rotation']")));
        QVERIFY(ui.contains(QStringLiteral("keyframableParameters: ['transition.rotation']")));
        QVERIFY(ui.contains(QStringLiteral("_dh *= profile.height")));
        QVERIFY(ui.contains(QStringLiteral("widget === 'point' || widget === 'size'")));
        const int completed = ui.indexOf(QStringLiteral("Component.onCompleted"));
        const int preset = ui.indexOf(QStringLiteral("onPresetSelected"));
        const int blocked = ui.indexOf(QStringLiteral("filter.blockSignals = true"), completed);
        const int seeded = ui.indexOf(QStringLiteral("filter.set('_shotcut:startValue'"), completed);
        const int unblocked = ui.indexOf(QStringLiteral("filter.blockSignals = false"), seeded);
        const int seededAgain = ui.indexOf(QStringLiteral("filter.set('_shotcut:startValue'"),
                                           preset);
        QVERIFY(completed >= 0);
        QVERIFY(blocked > completed);
        QVERIFY(seeded > blocked);
        QVERIFY(unblocked > seeded);
        QVERIFY(unblocked < preset);
        QVERIFY(seededAgain > preset);
        const int presetInit = ui.indexOf(QStringLiteral("initializeSimpleKeyframes();"), preset);
        QVERIFY(presetInit > preset);
        const int presetSignals = ui.indexOf(QStringLiteral("filter.blockSignals"), preset);
        QVERIFY(presetSignals > presetInit);
        QVERIFY(ui.contains(QStringLiteral("id: rectangleX")));
        QVERIFY(ui.contains(QStringLiteral("id: rectangleY")));
        QVERIFY(ui.contains(QStringLiteral("id: rectangleW")));
        QVERIFY(ui.contains(QStringLiteral("id: rectangleH")));
        QVERIFY(ui.contains(QStringLiteral("id: rectangleKeyframesButton")));
        QVERIFY(ui.contains(QStringLiteral("Layout.rowSpan: 2")));
        QVERIFY(ui.contains(QStringLiteral("Layout.columnSpan: 1")));
        QVERIFY(ui.contains(QStringLiteral("rectangleKeyframesButton.checked")));
        const int positionRow = ui.indexOf(QStringLiteral("id: rectangleX"));
        const int positionUndo = ui.indexOf(QStringLiteral("rectangleX.value = defaults.x"));
        const int keyframesButton = ui.indexOf(QStringLiteral("id: rectangleKeyframesButton"));
        const int sizeRow = ui.indexOf(QStringLiteral("id: rectangleW"));
        QVERIFY(positionRow > 0);
        QVERIFY(positionUndo > positionRow);
        QVERIFY(keyframesButton > positionUndo);
        QVERIFY(sizeRow > keyframesButton);
        QVERIFY(!ui.contains(QStringLiteral("propertyName: 'anchor'")));
        QVERIFY(!ui.contains(QStringLiteral("propertyName: 'geometry'")));
        QVERIFY(ui.contains(QStringLiteral("id: param_3_x")));
        QVERIFY(ui.contains(QStringLiteral("id: param_3_y")));
        QVERIFY(ui.contains(QStringLiteral("id: param_3_w")));
        QVERIFY(ui.contains(QStringLiteral("id: param_3_h")));
        QVERIFY(ui.contains(QStringLiteral("id: param_3_keyframes")));
        const int anchorX = ui.indexOf(QStringLiteral("id: param_3_x"));
        const int anchorKeyframes = ui.indexOf(QStringLiteral("id: param_3_keyframes"));
        const int anchorSpan = ui.indexOf(QStringLiteral("Layout.rowSpan: 2"), anchorKeyframes);
        const int anchorToggle = ui.indexOf(QStringLiteral("onClicked:"), anchorKeyframes);
        const int anchorW = ui.indexOf(QStringLiteral("id: param_3_w"));
        QVERIFY(anchorX > sizeRow);
        QVERIFY(anchorKeyframes > anchorX);
        QVERIFY(anchorSpan > anchorKeyframes);
        QVERIFY(anchorToggle > anchorSpan);
        QVERIFY(anchorW > anchorKeyframes);
        const QString extraButton = ui.mid(anchorKeyframes, anchorW - anchorKeyframes);
        QVERIFY(extraButton.contains(QStringLiteral("checkable: true")));
        QVERIFY(!extraButton.contains(QStringLiteral("clearSimpleAnimation")));
        QVERIFY(ui.contains(QStringLiteral("function rectangleIsUnitless")));
        QVERIFY(ui.contains(QStringLiteral("root.rectangleIsUnitless(value)")));
        QVERIFY(ui.contains(QStringLiteral("root.rectangleIsUnitless(_value)")));
        QVERIFY(ui.contains(QStringLiteral("function extraRectPixels")));
        QVERIFY(ui.contains(QStringLiteral("function writeExtraRect")));
        QVERIFY(ui.contains(QStringLiteral("'shotcut:rectPixels:' + name")));
        QVERIFY(ui.contains(QStringLiteral("root.extraRectPixels('anchor', root.getPosition())")));
        QVERIFY(ui.contains(QStringLiteral("root.defaultRectanglePixels('anchor')")));
        QVERIFY(ui.contains(
            QStringLiteral("param_3_keyframes.checked = filter.keyframeCount('anchor') > 0")));
        QVERIFY(ui.contains(QStringLiteral("checked = filter.keyframeCount('anchor') > 0;")));
        QVERIFY(ui.contains(QStringLiteral("root.writeExtraRect('anchor'")));
        QVERIFY(ui.contains(QStringLiteral("names.push('shotcut:rectPixels:anchor')")));
        QVERIFY(ui.contains(QStringLiteral("_presetNames.push('shotcut:rectPixels:anchor')")));
        QVERIFY(ui.contains(QStringLiteral("filter.resetProperty('shotcut:rectPixels:anchor')")));
        QVERIFY(ui.contains(QStringLiteral("function consumeVuiRotation()")));
        QVERIFY(ui.contains(
            QStringLiteral("root.updateFilter(root.rectangleRotationProperty, value,")));
        QVERIFY(ui.contains(QStringLiteral(
            "filter.getDouble(root.rectangleRotationProperty, root.getPosition())")));
        QVERIFY(ui.contains(QStringLiteral("_shotcut:vuiRotation")));
        QVERIFY(ui.contains(QStringLiteral("shotcut:rectPixels")));
        QVERIFY(ui.contains(QStringLiteral("function rectangleComponentPixels")));
        QVERIFY(ui.contains(QStringLiteral("function defaultRectanglePixels")));
        QVERIFY(ui.contains(QStringLiteral("function rewriteRectangleKeyframes")));
        QVERIFY(ui.contains(QStringLiteral("text.charAt(text.length - 1) === '%'")));
        QVERIFY(ui.contains(QStringLiteral("text.match(/^([^:]*):([^x]*)x(.*)$/)")));
        QVERIFY(ui.contains(QStringLiteral("if (normalized && text.indexOf('%') < 0)")));
        QVERIFY(ui.contains(QStringLiteral("var opacityAt = heightToken.indexOf(':')")));
        QVERIFY(ui.contains(QStringLiteral("root.startValues[index] === undefined")));
        QVERIFY(ui.contains(QStringLiteral("root.endValues[index] === undefined")));
        QVERIFY(ui.contains(QStringLiteral("function convertRectangleKeyframesToPixels")));
        QVERIFY(ui.contains(QStringLiteral("filter.removeRectPercents(name)")));
        QVERIFY(ui.contains(QStringLiteral("filter.keyframeCount(name) < 1")));
        QVERIFY(ui.contains(QStringLiteral("filter.getNextKeyframePosition(name, -1)")));
        QVERIFY(ui.contains(QStringLiteral("filter.set('_shotcut:vuiRotation', '')")));
        const int created = ui.indexOf(QStringLiteral("if (filter.isNew)"));
        const int savePreset = ui.indexOf(QStringLiteral("filter.savePreset"), created);
        QVERIFY(created >= 0);
        QVERIFY(savePreset > created);
        const QString initBody = ui.mid(created, savePreset - created);
        QVERIFY(initBody.contains(QStringLiteral("root.defaultRectanglePixels(p)")));
        QVERIFY(initBody.contains(QStringLiteral("filter.set('shotcut:rectPixels', 1)")));
        QVERIFY(initBody.contains(QStringLiteral("filter.set(root.extraRectFlag(p), 1)")));
        const int defaultsFn = ui.indexOf(QStringLiteral("function defaultRectanglePixels"));
        const int rewriteFn = ui.indexOf(QStringLiteral("function rewriteRectangleKeyframes"));
        QVERIFY(defaultsFn >= 0);
        QVERIFY(rewriteFn > defaultsFn);
        const QString defaultsBody = ui.mid(defaultsFn, rewriteFn - defaultsFn);
        QVERIFY(defaultsBody.contains(QStringLiteral("propertyNormalizedDefault")));
        QVERIFY(!defaultsBody.contains(QStringLiteral("propertyNormalizedCoordinates")));
        QVERIFY(ui.contains(QStringLiteral("root.rewriteRectangleKeyframes()")));
        QVERIFY(ui.contains(QStringLiteral("names.push('shotcut:rectPixels')")));
        QVERIFY(ui.contains(QStringLiteral("filter.resetProperty('shotcut:rectPixels')")));
        QVERIFY(!vui.contains(QStringLiteral("rotationStartValue")));
    }

    void textRotationMirrorsTheTextField()
    {
        AddOnFilterDescriptor descriptor;
        descriptor.service = QStringLiteral("test.textrot");
        descriptor.title = QStringLiteral("Text rotation");
        descriptor.parameters << parameter(QStringLiteral("geometry"), QStringLiteral("rect"), true);
        descriptor.parameters << parameter(QStringLiteral("rotation"),
                                           QStringLiteral("float"),
                                           true,
                                           QStringLiteral("text"));

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString ui;
        QVERIFY(generate(descriptor, dir, &ui, nullptr, nullptr));
        QVERIFY(ui.contains(QStringLiteral("'rotation': 'text'")));
        QVERIFY(ui.contains(QStringLiteral("if (widget === 'text')")));
        QVERIFY(ui.contains(QStringLiteral(".text = String(value)")));
        QVERIFY(ui.contains(
            QStringLiteral("root.updateFilter(root.rectangleRotationProperty, value,")));
        QVERIFY(!ui.contains(
            QStringLiteral("root.updateFilter(root.rectangleRotationProperty, param_")));
    }

    void animationDisabledSkipsRectangleKeyframes()
    {
        AddOnFilterDescriptor descriptor;
        descriptor.service = QStringLiteral("test.static");
        descriptor.title = QStringLiteral("Static");
        descriptor.parameters << parameter(QStringLiteral("geometry"),
                                           QStringLiteral("rect"),
                                           false);
        descriptor.parameters << parameter(QStringLiteral("rotation"),
                                           QStringLiteral("float"),
                                           false);
        descriptor.parameters << parameter(QStringLiteral("anchor"), QStringLiteral("rect"), false);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString ui;
        QString vui;
        QString meta;
        QVERIFY(generate(descriptor, dir, &ui, &vui, &meta));
        const int completed = ui.indexOf(QStringLiteral("Component.onCompleted"));
        QVERIFY(completed >= 0);
        QCOMPARE(ui.indexOf(QStringLiteral("filter.blockSignals"), completed), -1);
        QCOMPARE(ui.indexOf(QStringLiteral("_shotcut:startValue"), completed), -1);
        QVERIFY(ui.contains(QStringLiteral("rectangleKeyframes: false")));
        QVERIFY(ui.contains(QStringLiteral("id: rectangleX")));
        QVERIFY(!ui.contains(QStringLiteral("rectangleKeyframesButton")));
        QVERIFY(ui.contains(QStringLiteral("id: param_2_x")));
        QVERIFY(ui.contains(QStringLiteral("id: param_2_w")));
        QVERIFY(!ui.contains(QStringLiteral("id: param_2_keyframes")));
        QVERIFY(!ui.contains(QStringLiteral("propertyName: 'anchor'")));
        QVERIFY(ui.contains(QStringLiteral("root.defaultRectanglePixels('anchor')")));
        QVERIFY(vui.contains(QStringLiteral("rectProperty: 'geometry'")));
        QVERIFY(!vui.contains(QStringLiteral("rectProperty: 'anchor'")));
        QVERIFY(vui.contains(QStringLiteral("rotationProperty: 'rotation'")));
        QVERIFY(vui.contains(QStringLiteral("rectKeyframes: false")));
        QVERIFY(vui.contains(QStringLiteral("rotationKeyframes: false")));
        QVERIFY(meta.contains(QStringLiteral("vui: 'vui.qml'")));
        QVERIFY(!meta.contains(QStringLiteral("keyframes")));
        QVERIFY(!meta.contains(QStringLiteral("isRectangle")));
    }

    void rotationKeyframesFollowTheirOwnFlag()
    {
        AddOnFilterDescriptor descriptor;
        descriptor.service = QStringLiteral("test.mixed");
        descriptor.title = QStringLiteral("Mixed");
        descriptor.parameters << parameter(QStringLiteral("geometry"), QStringLiteral("rect"), true);
        descriptor.parameters << parameter(QStringLiteral("rotation"),
                                           QStringLiteral("float"),
                                           false);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString ui;
        QString vui;
        QString meta;
        QVERIFY(generate(descriptor, dir, &ui, &vui, &meta));
        QVERIFY(vui.contains(QStringLiteral("rectKeyframes: true")));
        QVERIFY(vui.contains(QStringLiteral("rotationKeyframes: false")));
        QVERIFY(meta.contains(QStringLiteral("isRectangle: true")));
        QVERIFY(!meta.contains(QStringLiteral("property: 'rotation'")));
        QVERIFY(ui.contains(QStringLiteral("keyframableParameters: []")));
    }

    void audioFilterSkipsVui()
    {
        AddOnFilterDescriptor descriptor;
        descriptor.service = QStringLiteral("test.audio");
        descriptor.title = QStringLiteral("Audio");
        descriptor.isAudio = true;
        descriptor.parameters << parameter(QStringLiteral("geometry"), QStringLiteral("rect"), true);
        descriptor.parameters << parameter(QStringLiteral("rotation"),
                                           QStringLiteral("float"),
                                           true);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString meta;
        QVERIFY(generate(descriptor, dir, nullptr, nullptr, &meta));
        QVERIFY(!QFileInfo::exists(dir.filePath(QStringLiteral("vui.qml"))));
        QVERIFY(!meta.contains(QStringLiteral("vui:")));
    }
};

QTEST_APPLESS_MAIN(TestAddOnQmlGenerator)

#include "test_addonqmlgenerator.moc"
