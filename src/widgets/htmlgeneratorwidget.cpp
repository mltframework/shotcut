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
#include "qmltypes/colordialog.h"
#include "qmltypes/qmlapplication.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QMovie>
#include <QPixmap>
#include <QTemporaryFile>

static const QString kTransparent = QObject::tr("transparent", "New > Image/Video From HTML");
static QString kPresetsFolder("HtmlGenerator");
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

    // Enable mouse tracking for hover detection
    ui->presetIconView->setMouseTracking(true);
    ui->presetIconView->viewport()->setMouseTracking(true);

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
    populatePresetIconView();
}

HtmlGeneratorWidget::~HtmlGeneratorWidget()
{
    // Clean up movie objects
    qDeleteAll(m_iconMovies);
    m_iconMovies.clear();
    delete ui;
}

void HtmlGeneratorWidget::on_colorButton_clicked()
{
    const QColor color = colorStringToResource(ui->colorLabel->text());
    // At this point Qt's colors are misread from CSS and and are stored in Qt as ARGB instead of RGBA
    // Remap to Qt's color order RGBA
    const QColor qtColor(color.alpha(), color.red(), color.green(), color.blue());
    auto newColor = ColorDialog::getColor(qtColor, this);

    if (newColor.isValid()) {
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
    if (p->property_exists(kColorProperty))
        ui->stackedWidget->setCurrentIndex(1);
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
    if (checked)
        ui->bodyTextEdit->setFocus(Qt::PopupFocusReason);
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
    on_bodyToggleButton_toggled(!hasPlaceholders);

    // Show individual line fields based on what placeholders are in the body
    ui->line1Label->setVisible(body.contains("%1"));
    ui->line1LineEdit->setVisible(body.contains("%1"));
    ui->line2Label->setVisible(body.contains("%2"));
    ui->line2LineEdit->setVisible(body.contains("%2"));
    ui->line3Label->setVisible(body.contains("%3"));
    ui->line3LineEdit->setVisible(body.contains("%3"));
    if (ui->line1LineEdit->isVisible())
        ui->line1LineEdit->setFocus(Qt::PopupFocusReason);
}

void HtmlGeneratorWidget::populatePresetIconView()
{
    // Clean up existing movies
    qDeleteAll(m_iconMovies);
    m_iconMovies.clear();
    ui->presetIconView->clear();

    // Install event filter to detect when mouse leaves an item
    ui->presetIconView->viewport()->installEventFilter(this);

    // Get list of presets from the ServicePresetWidget directory
    QDir dir(Settings.appDataLocation());
    if (!dir.cd("presets"))
        return;

    // Get global presets
    QStringList presetNames;
    presetNames.append(dir.entryList(QDir::Files));

    // Get widget-specific presets
    if (dir.cd(kPresetsFolder)) {
        QStringList widgetPresets = dir.entryList(QDir::Files | QDir::Readable);
        for (const auto &preset : std::as_const(widgetPresets)) {
            // Exclude .webp files
            if (!preset.endsWith(".webp", Qt::CaseInsensitive))
                presetNames.append(preset);
        }
        dir.cdUp();
    }

    // Create icon items for each preset
    for (const auto &presetName : std::as_const(presetNames)) {
        if (presetName.isEmpty())
            continue;

        auto *item = new QListWidgetItem();
        item->setText(presetName == tr("(defaults)") ? tr("(custom)") : presetName);
        item->setData(Qt::UserRole, presetName);

        // Find and setup icon (animated or static)
        auto iconPath = findPresetIconPath(presetName);
        if (!iconPath.isEmpty()) {
            setupIconAnimation(item, iconPath);
        }

        ui->presetIconView->addItem(item);
    }
}

QString HtmlGeneratorWidget::findPresetIconPath(const QString &presetName)
{
    QString iconFileName = presetName + QStringLiteral(".webp");

    // First, try loading from preset directory
    QDir dir(Settings.appDataLocation());
    if (dir.cd("presets") && dir.cd(kPresetsFolder)) {
        QString iconPath = dir.filePath(iconFileName);
        if (QFile::exists(iconPath)) {
            return iconPath;
        }
        dir.cdUp();
        dir.cdUp();
    }

    // If not found, try loading from data directory
    auto dataDir = QmlApplication::dataDir();
    dataDir.cd("shotcut");
    dataDir.cd("html-icons");
    auto iconPath = dataDir.filePath(iconFileName);
    if (QFile::exists(iconPath)) {
        return iconPath;
    }

    // Return empty string if not found
    return QString();
}

void HtmlGeneratorWidget::setupIconAnimation(QListWidgetItem *item, const QString &iconPath)
{
    // Try to load as an animated image first
    auto *movie = new QMovie(iconPath, QByteArray(), this);

    if (movie->isValid() && movie->frameCount() > 1) {
        // It's an animated image
        m_iconMovies.append(movie);
        movie->setScaledSize(QSize(128, 128));

        // Store the movie pointer in the item for later access
        item->setData(Qt::UserRole + 1, QVariant::fromValue(movie));

        // Connect to update the icon whenever a new frame is available
        connect(movie, &QMovie::frameChanged, this, [this, item, movie]() {
            item->setIcon(QIcon(movie->currentPixmap()));
        });

        // Set initial frame but don't start animation yet
        movie->jumpToFrame(0);
        item->setIcon(QIcon(movie->currentPixmap()));
    } else {
        // It's a static image or movie is invalid
        delete movie;
        QPixmap pixmap(iconPath);
        if (!pixmap.isNull()) {
            item->setIcon(
                QIcon(pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
    }
}

void HtmlGeneratorWidget::on_presetIconView_itemClicked(QListWidgetItem *item)
{
    if (!item)
        return;

    auto presetName = item->data(Qt::UserRole).toString();

    // Load the preset
    QDir dir(Settings.appDataLocation());
    if (!dir.cd("presets") || !dir.cd(kPresetsFolder))
        return;

    auto presetPath = dir.filePath(presetName);
    if (!QFile::exists(presetPath))
        return;

    // Load properties from preset file
    std::unique_ptr<Mlt::Properties> properties{
        Mlt::Properties::parse_yaml(presetPath.toUtf8().constData())};
    if (!properties)
        return;
    loadPreset(*properties);

    // Update the preset combo to reflect the selected preset
    ui->preset->setPreset(presetName);

    // Switch to editor page
    ui->stackedWidget->setCurrentIndex(1);
}

bool HtmlGeneratorWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->presetIconView->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            auto mouseEvent = static_cast<QMouseEvent *>(event);
            QListWidgetItem *item = ui->presetIconView->itemAt(mouseEvent->pos());

            if (item != m_lastHoveredItem) {
                // Stop animation on previously hovered item
                if (m_lastHoveredItem) {
                    auto prevMovie = m_lastHoveredItem->data(Qt::UserRole + 1).value<QMovie *>();
                    if (prevMovie && prevMovie->state() == QMovie::Running) {
                        prevMovie->stop();
                        prevMovie->jumpToFrame(0);
                        m_lastHoveredItem->setIcon(QIcon(prevMovie->currentPixmap()));
                    }
                }

                // Start animation on newly hovered item
                if (item) {
                    auto movie = item->data(Qt::UserRole + 1).value<QMovie *>();
                    if (movie && movie->frameCount() > 1) {
                        movie->start();
                    }
                }

                m_lastHoveredItem = item;
            }
        } else if (event->type() == QEvent::Leave) {
            // Stop animation when mouse leaves the widget
            if (m_lastHoveredItem) {
                auto movie = m_lastHoveredItem->data(Qt::UserRole + 1).value<QMovie *>();
                if (movie && movie->state() == QMovie::Running) {
                    movie->stop();
                    movie->jumpToFrame(0);
                    m_lastHoveredItem->setIcon(QIcon(movie->currentPixmap()));
                }
                m_lastHoveredItem = nullptr;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}
