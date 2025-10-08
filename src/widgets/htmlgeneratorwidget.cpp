/*
 * Copyright (c) 2025 Meltytech, LLC
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

#include "htmlgeneratorwidget.h"
#include "ui_htmlgeneratorwidget.h"

#include "Logger.h"
#include "dialogs/durationdialog.h"
#include "htmlgenerator.h"
#include "jobqueue.h"
#include "jobs/htmlgeneratorjob.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "qmltypes/qmlapplication.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QTemporaryFile>

static const QString kTransparent = QObject::tr("transparent", "New > Image/Video From HTML");
const char *HtmlGeneratorWidget::kColorProperty = "shotcut:color";
const char *HtmlGeneratorWidget::kCssProperty = "shotcut:css";
const char *HtmlGeneratorWidget::kBodyProperty = "shotcut:body";
const char *HtmlGeneratorWidget::kJavaScriptProperty = "shotcut:javascript";
const char *HtmlGeneratorWidget::kLine1Property = "shotcut:line1";
const char *HtmlGeneratorWidget::kLine2Property = "shotcut:line2";
const char *HtmlGeneratorWidget::kLine3Property = "shotcut:line3";

static QString colorToString(const QColor &color)
{
    return (color == QColor(0, 0, 0, 0)) ? kTransparent
                                         : QString::asprintf("#%02X%02X%02X%02X",
                                                             qRed(color.rgba()),
                                                             qGreen(color.rgba()),
                                                             qBlue(color.rgba()),
                                                             qAlpha(color.rgba()));
}

static QString colorStringToResource(const QString &s)
{
    return (s == kTransparent) ? "#00000000" : s;
}

HtmlGeneratorWidget::HtmlGeneratorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HtmlGeneratorWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label_2);
    ui->colorLabel->setText(kTransparent);
    updateTextSectionVisibility();
    on_bodyToggleButton_toggled(false);
    on_cssToggleButton_toggled(false);
    on_javascriptToggleButton_toggled(false);
    ui->preset->saveDefaultPreset(getPreset());

    Mlt::Properties p;
    QFile f;

    f.setFileName(QStringLiteral(":/resources/html/elastic_stroke/css"));
    f.open(QIODevice::ReadOnly);
    p.set(kCssProperty, f.readAll().toBase64().constData());
    f.close();
    f.setFileName(QStringLiteral(":/resources/html/elastic_stroke/body.html"));
    f.open(QIODevice::ReadOnly);
    p.set(kBodyProperty, f.readAll().toBase64().constData());
    f.close();
    p.set(kColorProperty, colorStringToResource(kTransparent).toLatin1().constData());
    ui->preset->savePreset(p, tr("Elastic Stroke (Video)"));

    f.setFileName(QStringLiteral(":/resources/html/folded/css"));
    f.open(QIODevice::ReadOnly);
    p.set(kCssProperty, f.readAll().toBase64().constData());
    f.close();
    f.setFileName(QStringLiteral(":/resources/html/folded/folded.js"));
    f.open(QIODevice::ReadOnly);
    p.set(kJavaScriptProperty, f.readAll().toBase64().constData());
    f.close();
    f.setFileName(QStringLiteral(":/resources/html/folded/body.html"));
    f.open(QIODevice::ReadOnly);
    p.set(kBodyProperty, f.readAll().toBase64().constData());
    f.close();
    p.set(kColorProperty, colorStringToResource(kTransparent).toLatin1().constData());
    ui->preset->savePreset(p, tr("Folded (Image)"));

    f.setFileName(QStringLiteral(":/resources/html/gold_metal/css"));
    f.open(QIODevice::ReadOnly);
    p.set(kCssProperty, f.readAll().toBase64().constData());
    f.close();
    f.setFileName(QStringLiteral(":/resources/html/gold_metal/body.html"));
    f.open(QIODevice::ReadOnly);
    p.set(kBodyProperty, f.readAll().toBase64().constData());
    f.close();
    p.set(kColorProperty, colorStringToResource(kTransparent).toLatin1().constData());
    ui->preset->savePreset(p, tr("Gold Metal (Image)"));

    f.setFileName(QStringLiteral(":/resources/html/party_time/css"));
    f.open(QIODevice::ReadOnly);
    p.set(kCssProperty, f.readAll().toBase64().constData());
    f.close();
    f.setFileName(QStringLiteral(":/resources/html/party_time/party_time.js"));
    f.open(QIODevice::ReadOnly);
    p.set(kJavaScriptProperty, f.readAll().toBase64().constData());
    f.close();
    f.setFileName(QStringLiteral(":/resources/html/party_time/body.html"));
    f.open(QIODevice::ReadOnly);
    p.set(kBodyProperty, f.readAll().toBase64().constData());
    f.close();
    p.set(kColorProperty, colorStringToResource(kTransparent).toLatin1().constData());
    ui->preset->savePreset(p, tr("Party Time (Video)"));

    f.setFileName(QStringLiteral(":/resources/html/three_d/css"));
    f.open(QIODevice::ReadOnly);
    p.set(kCssProperty, f.readAll().toBase64().constData());
    f.close();
    f.setFileName(QStringLiteral(":/resources/html/three_d/body.html"));
    f.open(QIODevice::ReadOnly);
    p.set(kBodyProperty, f.readAll().toBase64().constData());
    f.close();
    p.set(kColorProperty, colorStringToResource(kTransparent).toLatin1().constData());
    ui->preset->savePreset(p, tr("3D (Image)"));

    ui->preset->loadPresets();
}

HtmlGeneratorWidget::~HtmlGeneratorWidget()
{
    delete ui;
}

void HtmlGeneratorWidget::on_colorButton_clicked()
{
    const QColor color = colorStringToResource(ui->colorLabel->text());
    // At this point Qt's colors are misread from CSS and and are stored in Qt as ARGB instead of RGBA
    QColorDialog::ColorDialogOptions flags = QColorDialog::ShowAlphaChannel;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    flags = flags | QColorDialog::DontUseNativeDialog;
#endif
    // Remap to Qt's color order RGBA
    const QColor qtColor(color.alpha(), color.red(), color.green(), color.blue());
    auto newColor = QColorDialog::getColor(qtColor, this, QString(), flags);
    if (newColor.isValid()) {
        auto rgb = newColor;
        auto transparent = QColor(0, 0, 0, 0);
        rgb.setAlpha(color.alpha());
        if (newColor.alpha() == 0
            && (rgb != color || (newColor == transparent && color == transparent))) {
            newColor.setAlpha(255);
        }
        ui->colorLabel->setText(colorToString(newColor));
        ui->colorLabel->setStyleSheet(QStringLiteral("color: %1; background-color: %2")
                                          .arg(Util::textColor(newColor), newColor.name()));
    }
}

Mlt::Producer *HtmlGeneratorWidget::newProducer(Mlt::Profile &profile)
{
    auto *p = new Mlt::Producer(profile, "color:");
    p->set("resource", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());
    p->set("mlt_image_format", "rgba");
    MLT.setDurationFromDefault(p);
    p->set(kShotcutCaptionProperty, ui->colorLabel->text().toLatin1().constData());
    p->set(kShotcutDetailProperty, ui->colorLabel->text().toLatin1().constData());
    return p;
}

Mlt::Properties HtmlGeneratorWidget::getPreset() const
{
    Mlt::Properties p;
    const QString color = colorStringToResource(ui->colorLabel->text());
    p.set(kColorProperty, color.toLatin1().constData());
    p.set(kCssProperty, ui->cssTextEdit->toPlainText().toUtf8().toBase64().constData());
    p.set(kBodyProperty, ui->bodyTextEdit->toPlainText().toUtf8().toBase64().constData());
    p.set(kJavaScriptProperty,
          ui->javascriptTextEdit->toPlainText().toUtf8().toBase64().constData());
    p.set(kLine1Property, ui->line1LineEdit->text().toUtf8().toBase64().constData());
    p.set(kLine2Property, ui->line2LineEdit->text().toUtf8().toBase64().constData());
    p.set(kLine3Property, ui->line3LineEdit->text().toUtf8().toBase64().constData());
    return p;
}

void HtmlGeneratorWidget::loadPreset(Mlt::Properties &p)
{
    const auto mltColor = p.get_color(kColorProperty);
    const QColor color(mltColor.r, mltColor.g, mltColor.b, mltColor.a);
    ui->colorLabel->setText(colorToString(color));
    ui->colorLabel->setStyleSheet(
        QStringLiteral("color: %1; background-color: %2").arg(Util::textColor(color), color.name()));
    const auto css = QByteArray::fromBase64(p.get(kCssProperty));
    ui->cssTextEdit->setPlainText(QString::fromUtf8(css));
    const auto body = QByteArray::fromBase64(p.get(kBodyProperty));
    ui->bodyTextEdit->setPlainText(QString::fromUtf8(body));
    const auto javascript = QByteArray::fromBase64(p.get(kJavaScriptProperty));
    ui->javascriptTextEdit->setPlainText(QString::fromUtf8(javascript));
    const auto line1 = QByteArray::fromBase64(p.get(kLine1Property));
    ui->line1LineEdit->setText(QString::fromUtf8(line1));
    const auto line2 = QByteArray::fromBase64(p.get(kLine2Property));
    ui->line2LineEdit->setText(QString::fromUtf8(line2));
    const auto line3 = QByteArray::fromBase64(p.get(kLine3Property));
    ui->line3LineEdit->setText(QString::fromUtf8(line3));
    updateTextSectionVisibility();
    if (ui->line1Label->isVisible())
        ui->line1LineEdit->setFocus();
}

void HtmlGeneratorWidget::setProducer(Mlt::Producer *p)
{
    AbstractProducerWidget::setProducer(p);
    if (!p || !p->is_valid())
        return;
    const auto mltColor = p->property_exists(kColorProperty) ? p->get_color(kColorProperty)
                                                             : mlt_color({0, 0, 0, 0});
    const QColor qtColor(mltColor.r, mltColor.g, mltColor.b, mltColor.a);
    ui->colorLabel->setText(colorToString(qtColor));
    ui->colorLabel->setStyleSheet(QStringLiteral("color: %1; background-color: %2")
                                      .arg(Util::textColor(qtColor), qtColor.name()));
    const auto css = QByteArray::fromBase64(p->get(kCssProperty));
    ui->cssTextEdit->setPlainText(QString::fromUtf8(css));
    const auto body = QByteArray::fromBase64(p->get(kBodyProperty));
    ui->bodyTextEdit->setPlainText(QString::fromUtf8(body));
    const auto javascript = QByteArray::fromBase64(p->get(kJavaScriptProperty));
    ui->javascriptTextEdit->setPlainText(QString::fromUtf8(javascript));
    const auto line1 = QByteArray::fromBase64(p->get(kLine1Property));
    ui->line1LineEdit->setText(QString::fromUtf8(line1));
    const auto line2 = QByteArray::fromBase64(p->get(kLine2Property));
    ui->line2LineEdit->setText(QString::fromUtf8(line2));
    const auto line3 = QByteArray::fromBase64(p->get(kLine3Property));
    ui->line3LineEdit->setText(QString::fromUtf8(line3));
    updateTextSectionVisibility();
}

void HtmlGeneratorWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void HtmlGeneratorWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}

QString HtmlGeneratorWidget::generateHtml() const
{
    QString html(R"(
<html>
  <head>
    <style>%1</style>
  </head>
  <body style="margin: 0; background: %3">
%4
  </body>
  <script>%2</script>
</html>
)");
    // Format the body text with line inputs using QString::arg()
    auto formattedBody = ui->bodyTextEdit->toPlainText();
    if (formattedBody.contains("%3")) {
        formattedBody = formattedBody.arg(ui->line1LineEdit->text(),
                                          ui->line2LineEdit->text(),
                                          ui->line3LineEdit->text());
    } else if (formattedBody.contains("%2")) {
        formattedBody = formattedBody.arg(ui->line1LineEdit->text(), ui->line2LineEdit->text());
    } else if (formattedBody.contains("%1")) {
        formattedBody = formattedBody.arg(ui->line1LineEdit->text());
    }

    return html.arg(ui->cssTextEdit->toPlainText(),
                    ui->javascriptTextEdit->toPlainText(),
                    ui->colorLabel->text(),
                    formattedBody);
}

void HtmlGeneratorWidget::on_imageButton_clicked()
{
    auto outputPath = QmlApplication::getNextProjectFile("html-.png");
    if (outputPath.isEmpty()) {
        outputPath = QFileDialog::getSaveFileName(this,
                                                  tr("Generate Image File"),
                                                  Settings.savePath(),
                                                  tr("PNG files (*.png)"));
        if (outputPath.isEmpty())
            return;
        if (!outputPath.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) {
            outputPath += QStringLiteral(".png");
        }
        Settings.setSavePath(QFileInfo(outputPath).path());
    }

    QFileInfo outInfo(outputPath);
    auto txtFile = new QTemporaryFile(outInfo.dir().filePath("XXXXXX.html"), this);

    if (!txtFile->open()) {
        LOG_ERROR() << "Failed to create temp HTML file" << txtFile->fileName();
        return;
    }
    const auto html = generateHtml();
    txtFile->write(html.toUtf8());
    txtFile->close();

    auto generator = new HtmlGenerator(this);
    connect(
        generator,
        &HtmlGenerator::imageReady,
        this,
        [=](QString outputPath) {
            auto p = new Mlt::Producer(MLT.profile(), outputPath.toUtf8().constData());
            QString color = colorStringToResource(ui->colorLabel->text());
            p->pass_property(*m_producer, kPrivateProducerProperty);
            p->set(kColorProperty, color.toLatin1().constData());
            p->set(kCssProperty, ui->cssTextEdit->toPlainText().toUtf8().toBase64().constData());
            p->set(kBodyProperty, ui->bodyTextEdit->toPlainText().toUtf8().toBase64().constData());
            p->set(kJavaScriptProperty,
                   ui->javascriptTextEdit->toPlainText().toUtf8().toBase64().constData());
            p->set(kLine1Property, ui->line1LineEdit->text().toUtf8().toBase64().constData());
            p->set(kLine2Property, ui->line2LineEdit->text().toUtf8().toBase64().constData());
            p->set(kLine3Property, ui->line3LineEdit->text().toUtf8().toBase64().constData());
            MLT.setImageDurationFromDefault(p);
            MAIN.open(p, false);
        },
        Qt::QueuedConnection);
    const QString url("file://" + txtFile->fileName());
    const QSize size(qRound(MLT.profile().width() * MLT.profile().sar()), MLT.profile().height());
    generator->launchBrowser(Settings.chromiumPath(), url, size, outputPath);
    MAIN.showStatusMessage(tr("Generating image..."), 3);
}

void HtmlGeneratorWidget::on_pushButton_clicked()
{
    ui->colorLabel->setText(kTransparent);
}

void HtmlGeneratorWidget::on_videoButton_clicked()
{
    // Show duration dialog first
    DurationDialog durationDialog(this);
    durationDialog.setDuration(MLT.profile().fps() * 10.0);
    if (durationDialog.exec() != QDialog::Accepted) {
        return;
    }

    int durationFrames = durationDialog.duration();
    int durationMs = (durationFrames / MLT.profile().fps()) * 1000.0;

    // Get save filename
    auto outputPath = QmlApplication::getNextProjectFile("html-.avi");
    if (outputPath.isEmpty()) {
        outputPath = QFileDialog::getSaveFileName(this,
                                                  tr("Generate Video File"),
                                                  Settings.savePath(),
                                                  tr("AVI files (*.avi)"));
        if (outputPath.isEmpty())
            return;

        if (!outputPath.endsWith(QStringLiteral(".avi"), Qt::CaseInsensitive)) {
            outputPath += QStringLiteral(".avi");
        }

        Settings.setSavePath(QFileInfo(outputPath).path());
    }

    // Create and add the job
    const auto job
        = new HtmlGeneratorJob(tr("Generate HTML Video: %1").arg(QFileInfo(outputPath).fileName()),
                               generateHtml(),
                               outputPath,
                               durationMs);
    QString color = colorStringToResource(ui->colorLabel->text());
    job->setProperty(kPrivateProducerProperty,
                     QString::fromLatin1(m_producer->get(kPrivateProducerProperty)));
    job->setProperty(kColorProperty, color.toLatin1());
    job->setProperty(kCssProperty, ui->cssTextEdit->toPlainText().toUtf8().toBase64().constData());
    job->setProperty(kBodyProperty, ui->bodyTextEdit->toPlainText().toUtf8().toBase64().constData());
    job->setProperty(kJavaScriptProperty,
                     ui->javascriptTextEdit->toPlainText().toUtf8().toBase64().constData());
    job->setProperty(kLine1Property, ui->line1LineEdit->text().toUtf8().toBase64().constData());
    job->setProperty(kLine2Property, ui->line2LineEdit->text().toUtf8().toBase64().constData());
    job->setProperty(kLine3Property, ui->line3LineEdit->text().toUtf8().toBase64().constData());
    JOBS.add(job);

    MAIN.showStatusMessage(tr("Generating HTML video..."), 3);
}

void HtmlGeneratorWidget::on_cssToggleButton_toggled(bool checked)
{
    ui->cssTextEdit->setVisible(checked);
    ui->cssToggleButton->setText(checked ? tr("▼ CSS") : tr("▶ CSS"));
}

void HtmlGeneratorWidget::on_bodyToggleButton_toggled(bool checked)
{
    ui->colorButton->setVisible(checked);
    ui->colorLabel->setVisible(checked);
    ui->pushButton->setVisible(checked);
    ui->bodySpacerLabel->setVisible(checked);
    ui->bodyTextEdit->setVisible(checked);
    ui->bodyToggleButton->setText(checked ? tr("▼ Body") : tr("▶ Body"));
}

void HtmlGeneratorWidget::on_javascriptToggleButton_toggled(bool checked)
{
    ui->javascriptTextEdit->setVisible(checked);
    ui->javascriptToggleButton->setText(checked ? tr("▼ JavaScript") : tr("▶ JavaScript"));
}

void HtmlGeneratorWidget::on_bodyTextEdit_textChanged()
{
    updateTextSectionVisibility();
}

void HtmlGeneratorWidget::updateTextSectionVisibility()
{
    const auto body = ui->bodyTextEdit->toPlainText();
    bool hasPlaceholders = body.contains("%1") || body.contains("%2") || body.contains("%3");

    ui->textLabel->setVisible(hasPlaceholders);

    // Show individual line fields based on what placeholders are in the body
    ui->line1Label->setVisible(body.contains("%1"));
    ui->line1LineEdit->setVisible(body.contains("%1"));
    ui->line2Label->setVisible(body.contains("%2"));
    ui->line2LineEdit->setVisible(body.contains("%2"));
    ui->line3Label->setVisible(body.contains("%3"));
    ui->line3LineEdit->setVisible(body.contains("%3"));
}
