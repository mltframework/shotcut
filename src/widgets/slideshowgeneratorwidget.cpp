/*
 * Copyright (c) 2020-2023 Meltytech, LLC
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

#include "slideshowgeneratorwidget.h"

#include "Logger.h"
#include "mltcontroller.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "widgets/producerpreviewwidget.h"
#include "qmltypes/qmlapplication.h"

#include <QComboBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QtConcurrent/QtConcurrent>

#include <math.h>

enum {
    ASPECT_CONVERSION_PAD_BLACK,
    ASPECT_CONVERSION_CROP_CENTER,
    ASPECT_CONVERSION_CROP_PAN,
    ASPECT_CONVERSION_PAD_BLUR,
};

static const int minTransitionFrames = 2;

SlideshowGeneratorWidget::SlideshowGeneratorWidget(Mlt::Playlist *clips, QWidget *parent)
    : QWidget(parent)
    , m_clips(clips)
    , m_refreshPreview(false)
{
    QGridLayout *grid = new QGridLayout();
    setLayout(grid);

    grid->addWidget(new QLabel(tr("Clip duration")), 0, 0, Qt::AlignRight);
    m_clipDurationSpinner = new QDoubleSpinBox();
    m_clipDurationSpinner->setToolTip(tr("Set the duration of each clip in the slideshow."));
    m_clipDurationSpinner->setSuffix(" s");
    m_clipDurationSpinner->setDecimals(1);
    m_clipDurationSpinner->setMinimum(0.2);
    m_clipDurationSpinner->setMaximum(3600 * 4);
    m_clipDurationSpinner->setValue(10);
    connect(m_clipDurationSpinner, SIGNAL(valueChanged(double)), this, SLOT(on_parameterChanged()));
    grid->addWidget(m_clipDurationSpinner, 0, 1);

    grid->addWidget(new QLabel(tr("Aspect ratio conversion")), 1, 0, Qt::AlignRight);
    m_aspectConversionCombo = new QComboBox();
    m_aspectConversionCombo->addItem(tr("Pad Black"));
    m_aspectConversionCombo->addItem(tr("Crop Center"));
    m_aspectConversionCombo->addItem(tr("Crop and Pan"));
    {
        QScopedPointer<Mlt::Properties> mltFilters(MLT.repository()->filters());
        if (mltFilters && mltFilters->property_exists("pillar_echo")) {
            m_aspectConversionCombo->addItem(tr("Pad Blur"));
        }
    }
    m_aspectConversionCombo->setToolTip(tr("Choose an aspect ratio conversion method."));
    m_aspectConversionCombo->setCurrentIndex(ASPECT_CONVERSION_CROP_CENTER);
    connect(m_aspectConversionCombo, SIGNAL(currentIndexChanged(int)), this,
            SLOT(on_parameterChanged()));
    grid->addWidget(m_aspectConversionCombo, 1, 1);

    grid->addWidget(new QLabel(tr("Zoom effect")), 2, 0, Qt::AlignRight);
    m_zoomPercentSpinner = new QSpinBox();
    m_zoomPercentSpinner->setToolTip(
        tr("Set the percentage of the zoom-in effect.\n0% will result in no zoom effect."));
    m_zoomPercentSpinner->setSuffix(" %");
    m_zoomPercentSpinner->setMinimum(-50);
    m_zoomPercentSpinner->setMaximum(50);
    m_zoomPercentSpinner->setValue(10);
    connect(m_zoomPercentSpinner, SIGNAL(valueChanged(int)), this, SLOT(on_parameterChanged()));
    grid->addWidget(m_zoomPercentSpinner, 2, 1);

    grid->addWidget(new QLabel(tr("Transition duration")), 3, 0, Qt::AlignRight);
    m_transitionDurationSpinner = new QDoubleSpinBox();
    m_transitionDurationSpinner->setToolTip(
        tr("Set the duration of the transition.\nMay not be longer than half the duration of the clip.\nIf the duration is 0, no transition will be created."));
    m_transitionDurationSpinner->setSuffix(" s");
    m_transitionDurationSpinner->setDecimals(1);
    m_transitionDurationSpinner->setMinimum(0);
    m_transitionDurationSpinner->setMaximum(10);
    m_transitionDurationSpinner->setValue(2);
    connect(m_transitionDurationSpinner, SIGNAL(valueChanged(double)), this,
            SLOT(on_parameterChanged()));
    grid->addWidget(m_transitionDurationSpinner, 3, 1);

    grid->addWidget(new QLabel(tr("Transition type")), 4, 0, Qt::AlignRight);
    m_transitionStyleCombo = new QComboBox();
    m_transitionStyleCombo->setMaximumWidth(350);
    m_transitionStyleCombo->addItem(tr("Random"));
    m_transitionStyleCombo->addItem(tr("Dissolve"));
    m_transitionStyleCombo->addItem(tr("Bar Horizontal"));
    m_transitionStyleCombo->addItem(tr("Bar Vertical"));
    m_transitionStyleCombo->addItem(tr("Barn Door Horizontal"));
    m_transitionStyleCombo->addItem(tr("Barn Door Vertical"));
    m_transitionStyleCombo->addItem(tr("Barn Door Diagonal SW-NE"));
    m_transitionStyleCombo->addItem(tr("Barn Door Diagonal NW-SE"));
    m_transitionStyleCombo->addItem(tr("Diagonal Top Left"));
    m_transitionStyleCombo->addItem(tr("Diagonal Top Right"));
    m_transitionStyleCombo->addItem(tr("Matrix Waterfall Horizontal"));
    m_transitionStyleCombo->addItem(tr("Matrix Waterfall Vertical"));
    m_transitionStyleCombo->addItem(tr("Matrix Snake Horizontal"));
    m_transitionStyleCombo->addItem(tr("Matrix Snake Parallel Horizontal"));
    m_transitionStyleCombo->addItem(tr("Matrix Snake Vertical"));
    m_transitionStyleCombo->addItem(tr("Matrix Snake Parallel Vertical"));
    m_transitionStyleCombo->addItem(tr("Barn V Up"));
    m_transitionStyleCombo->addItem(tr("Iris Circle"));
    m_transitionStyleCombo->addItem(tr("Double Iris"));
    m_transitionStyleCombo->addItem(tr("Iris Box"));
    m_transitionStyleCombo->addItem(tr("Box Bottom Right"));
    m_transitionStyleCombo->addItem(tr("Box Bottom Left"));
    m_transitionStyleCombo->addItem(tr("Box Right Center"));
    m_transitionStyleCombo->addItem(tr("Clock Top"));
    for (auto &s : QmlApplication::wipes()) {
        m_transitionStyleCombo->addItem(QFileInfo(s).fileName(), s);
    }
    m_transitionStyleCombo->setToolTip(tr("Choose a transition effect."));
    m_transitionStyleCombo->setCurrentIndex(1);
    connect(m_transitionStyleCombo, SIGNAL(currentIndexChanged(int)), this,
            SLOT(on_parameterChanged()));
    grid->addWidget(m_transitionStyleCombo, 4, 1);

    grid->addWidget(new QLabel(tr("Transition softness")), 5, 0, Qt::AlignRight);
    m_softnessSpinner = new QSpinBox();
    m_softnessSpinner->setToolTip(tr("Change the softness of the edge of the wipe."));
    m_softnessSpinner->setSuffix(" %");
    m_softnessSpinner->setMaximum(100);
    m_softnessSpinner->setMinimum(0);
    m_softnessSpinner->setValue(20);
    connect(m_softnessSpinner, SIGNAL(valueChanged(int)), this, SLOT(on_parameterChanged()));
    grid->addWidget(m_softnessSpinner, 5, 1);

    m_preview = new ProducerPreviewWidget(MLT.profile().dar());
    grid->addWidget(m_preview, 6, 0, 1, 2, Qt::AlignCenter);

    on_parameterChanged();
}

SlideshowGeneratorWidget::~SlideshowGeneratorWidget()
{
    m_future.waitForFinished();
    m_preview->stop();
}

Mlt::Playlist *SlideshowGeneratorWidget::getSlideshow()
{
    SlideshowConfig config;
    m_mutex.lock();
    // take a snapshot of the config.
    config = m_config;
    m_mutex.unlock();

    int framesPerClip = round(config.clipDuration * MLT.profile().fps());
    int count = m_clips->count();
    Mlt::Playlist *slideshow = new Mlt::Playlist(MLT.profile());
    Mlt::ClipInfo info;

    // Copy clips
    for (int i = 0; i < count; i++) {
        Mlt::ClipInfo *c = m_clips->clip_info(i, &info);
        if (c) {
            Mlt::Producer producer(MLT.profile(), "xml-string", MLT.XML(c->producer).toUtf8().constData());
            slideshow->append(producer, c->frame_in, c->frame_in + framesPerClip - 1);
        }
    }

    // Add filters
    for (int i = 0; i < count; i++) {
        Mlt::ClipInfo *c = slideshow->clip_info(i, &info);
        if (c && c->producer) {
            if (!c->producer->property_exists("meta.media.width")) {
                delete c->producer->get_frame(); // makes avformat producer set meta.media.width and .height
            }
            attachAffineFilter(config, c->producer, c->frame_count - 1);
            attachBlurFilter(config, c->producer);
        }
    }

    // Add transitions
    int framesPerTransition = round(config.transitionDuration * MLT.profile().fps());
    if (framesPerTransition > (framesPerClip / 2 - 1)) {
        framesPerTransition = (framesPerClip / 2 - 1);
    }
    if (framesPerTransition < minTransitionFrames) {
        framesPerTransition = 0;
    }
    if (framesPerTransition > 0) {
        for (int i = 0; i < count - 1; i++) {
            Mlt::ClipInfo *c = slideshow->clip_info(i, &info);
            if (c->frame_count < framesPerTransition) {
                // Do not add a transition if the first clip is too short
                continue;
            }
            c = slideshow->clip_info(i + 1, &info);
            if (c->frame_count < framesPerTransition) {
                // Do not add a transition if the second clip is too short
                continue;
            }

            // Create playlist mix
            slideshow->mix(i, framesPerTransition);
            QScopedPointer<Mlt::Producer> producer(slideshow->get_clip(i + 1));
            if ( producer.isNull() ) {
                break;
            }
            producer->parent().set(kShotcutTransitionProperty, "lumaMix");

            // Add mix transition
            Mlt::Transition crossFade(MLT.profile(), "mix:-1");
            slideshow->mix_add(i + 1, &crossFade);

            // Add luma transition
            Mlt::Transition luma(MLT.profile(), Settings.playerGPU() ? "movit.luma_mix" : "luma");
            applyLumaTransitionProperties(&luma, config);
            slideshow->mix_add(i + 1, &luma);

            count++;
            i++;
        }
    }

    return slideshow;
}

void SlideshowGeneratorWidget::attachAffineFilter(SlideshowConfig &config, Mlt::Producer *producer,
                                                  int endPosition)
{
    if (config.zoomPercent == 0 &&
            config.aspectConversion != ASPECT_CONVERSION_CROP_CENTER &&
            config.aspectConversion != ASPECT_CONVERSION_CROP_PAN) {
        return;
    }

    mlt_rect beginRect;
    mlt_rect endRect;
    beginRect.x = 0;
    beginRect.y = 0;
    beginRect.w = MLT.profile().width();
    beginRect.h = MLT.profile().height();
    beginRect.o = 1;
    endRect.x = beginRect.x;
    endRect.y = beginRect.y;
    endRect.w = beginRect.w;
    endRect.h = beginRect.h;
    endRect.o = 1;

    double destDar = MLT.profile().dar();
    double sourceW = producer->get_double("meta.media.width");
    double sourceH = producer->get_double("meta.media.height");
    double sourceAr = producer->get_double("aspect_ratio");
    double sourceDar = destDar;
    if (sourceW && sourceH && sourceAr) {
        sourceDar = sourceW * sourceAr / sourceH;
    }
    if (sourceDar == destDar && config.zoomPercent == 0) {
        // Aspect ratios match and no zoom. No need for affine.
        return;
    }

    if (config.aspectConversion == ASPECT_CONVERSION_CROP_CENTER ||
            config.aspectConversion == ASPECT_CONVERSION_CROP_PAN) {
        if (sourceDar > destDar) {
            // Crop sides to fit height
            beginRect.w = (double)MLT.profile().width() * sourceDar / destDar;
            beginRect.h = MLT.profile().height();
            beginRect.y = 0;
            endRect.w = beginRect.w;
            endRect.h = beginRect.h;
            endRect.y = beginRect.y;
            if (config.aspectConversion == ASPECT_CONVERSION_CROP_CENTER) {
                beginRect.x = ((double)MLT.profile().width() - beginRect.w) / 2.0;
                endRect.x = beginRect.x;
            } else {
                beginRect.x = 0;
                endRect.x = (double)MLT.profile().width() - endRect.w;
            }
        } else if (destDar > sourceDar) {
            // Crop top and bottom to fit width.
            beginRect.w = MLT.profile().width();
            beginRect.h = (double)MLT.profile().height() * destDar / sourceDar;
            beginRect.x = 0;
            endRect.w = beginRect.w;
            endRect.h = beginRect.h;
            endRect.x = beginRect.x;
            if (config.aspectConversion == ASPECT_CONVERSION_CROP_CENTER) {
                beginRect.y = ((double)MLT.profile().height() - beginRect.h) / 2.0;
                endRect.y = beginRect.y;
            } else {
                beginRect.y = 0;
                endRect.y =  (double)MLT.profile().height() - endRect.h;
            }
        }
    } else {
        // Pad: modify rect to fit the aspect ratio of the source
        if (sourceDar > destDar) {
            beginRect.w = MLT.profile().width();
            beginRect.h = (double)MLT.profile().height() * destDar / sourceDar;
            beginRect.x = 0;
            beginRect.y = ((double)MLT.profile().height() - beginRect.h) / 2.0;
        } else if (destDar > sourceDar) {
            beginRect.w = (double)MLT.profile().width() * sourceDar / destDar;
            beginRect.h = MLT.profile().height();
            beginRect.x = ((double)MLT.profile().width() - beginRect.w) / 2.0;
            beginRect.y = 0;
        }
        endRect.w = beginRect.w;
        endRect.h = beginRect.h;
        endRect.y = beginRect.y;
        endRect.x = beginRect.x;
    }

    if (config.zoomPercent > 0) {
        double endScale = (double)config.zoomPercent / 100.0;
        endRect.x = endRect.x - (endScale * endRect.w / 2.0);
        endRect.y = endRect.y - (endScale * endRect.h / 2.0);
        endRect.w = endRect.w + (endScale * endRect.w);
        endRect.h = endRect.h + (endScale * endRect.h);
    } else if (config.zoomPercent < 0) {
        double beginScale = -1.0 * (double)config.zoomPercent / 100.0;
        beginRect.x = beginRect.x - (beginScale * beginRect.w / 2.0);
        beginRect.y = beginRect.y - (beginScale * beginRect.h / 2.0);
        beginRect.w = beginRect.w + (beginScale * beginRect.w);
        beginRect.h = beginRect.h + (beginScale * beginRect.h);
    }

    Mlt::Filter filter(MLT.profile(), Settings.playerGPU() ? "movit.rect" : "affine");
    if (Settings.playerGPU()) {
        filter.anim_set("rect", beginRect, 0);
        filter.anim_set("rect", endRect, endPosition);
        filter.set("fill", 1);
        filter.set("distort", 0);
        filter.set("valign", "middle");
        filter.set("halign", "center");
        filter.set(kShotcutFilterProperty, "movitSizePosition");
    } else {
        filter.anim_set("transition.rect", beginRect, 0);
        filter.anim_set("transition.rect", endRect, endPosition);
        filter.set("transition.fill", 1);
        filter.set("transition.distort", 0);
        filter.set("transition.valign", "middle");
        filter.set("transition.halign", "center");
        filter.set("transition.threads", 0);
        filter.set("background", "color:#000000");
        filter.set(kShotcutFilterProperty, "affineSizePosition");
    }
    filter.set(kShotcutAnimInProperty, producer->frames_to_time(endPosition + 1, mlt_time_clock));
    filter.set(kShotcutAnimOutProperty, producer->frames_to_time(0, mlt_time_clock));
    producer->attach(filter);
}

void SlideshowGeneratorWidget::attachBlurFilter(SlideshowConfig &config, Mlt::Producer *producer)
{
    if (config.aspectConversion != ASPECT_CONVERSION_PAD_BLUR) {
        return;
    }
    mlt_rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = MLT.profile().width();
    rect.h = MLT.profile().height();
    rect.o = 1;

    double destDar = MLT.profile().dar();
    double sourceW = producer->get_double("meta.media.width");
    double sourceH = producer->get_double("meta.media.height");
    double sourceAr = producer->get_double("aspect_ratio");
    double sourceDar = destDar;
    if ( sourceW && sourceH && sourceAr ) {
        sourceDar = sourceW * sourceAr / sourceH;
    }
    if (sourceDar == destDar) {
        // Aspect ratios match. No need for pad.
        return;
    }

    if (sourceDar > destDar) {
        // Blur top/bottom to pad.
        rect.h = MLT.profile().height() * destDar / sourceDar;
        rect.y = ((double)MLT.profile().height() - rect.h) / 2.0;
    } else if (destDar > sourceDar) {
        // Blur sides to pad.
        rect.w = MLT.profile().width() * sourceDar / destDar;
        rect.x = ((double)MLT.profile().width() - rect.w) / 2.0;
    }

    Mlt::Filter filter(MLT.profile(), "pillar_echo");
    filter.set("rect", rect);
    filter.set("blur", 4);
    filter.set(kShotcutFilterProperty, "blur_pad");
    producer->attach(filter);
}

void SlideshowGeneratorWidget::applyLumaTransitionProperties(Mlt::Transition *luma,
                                                             SlideshowConfig &config)
{
    int index = config.transitionStyle;

    if (index == 0) {
        // Random: pick any number other than 0
        index = rand() % m_transitionStyleCombo->count() + 1;
    }

    if (index == 1) {
        // Dissolve
        luma->set("resource", "");
    } else if (index <= 23) {
        luma->set("resource", QString("%luma%1.pgm").arg(index - 1, 2, 10,
                                                         QChar('0')).toLatin1().constData());
    } else {
        luma->set("resource", m_transitionStyleCombo->itemData(index).toString().toUtf8().constData());
    }
    luma->set("softness", config.transitionSoftness / 100.0);
    luma->set("progressive", 1);
    if (!Settings.playerGPU()) {
        luma->set("alpha_over", 1);
        luma->set("fix_background_alpha", 1);
    }
}

void SlideshowGeneratorWidget::on_parameterChanged()
{
    if (m_transitionDurationSpinner->value() > m_clipDurationSpinner->value() / 2 ) {
        m_transitionDurationSpinner->setValue(m_clipDurationSpinner->value() / 2);
    }
    if (m_transitionDurationSpinner->value() == 0) {
        m_transitionStyleCombo->setEnabled(false);
        m_softnessSpinner->setEnabled(false);
    } else if (m_transitionStyleCombo->currentIndex() == 1) {
        m_transitionStyleCombo->setEnabled(true);
        m_softnessSpinner->setEnabled(false);
    } else {
        m_transitionStyleCombo->setEnabled(true);
        m_softnessSpinner->setEnabled(true);
    }

    m_preview->stop();
    m_preview->showText(Settings.playerGPU() ? tr("Preview Not Available") :
                        tr("Generating Preview..."));
    m_mutex.lock();
    m_refreshPreview = true;
    m_config.clipDuration = m_clipDurationSpinner->value();
    m_config.aspectConversion = m_aspectConversionCombo->currentIndex();
    m_config.zoomPercent = m_zoomPercentSpinner->value();
    m_config.transitionDuration = m_transitionDurationSpinner->value();
    m_config.transitionStyle = m_transitionStyleCombo->currentIndex();
    m_config.transitionSoftness = m_softnessSpinner->value();
    if (m_future.isFinished() || m_future.isCanceled()) {
        // Generate the preview producer in another thread because it can take some time
        m_future = QtConcurrent::run(&SlideshowGeneratorWidget::generatePreviewSlideshow, this);
    }
    m_mutex.unlock();
}

void SlideshowGeneratorWidget::generatePreviewSlideshow()
{
    m_mutex.lock();
    while (m_refreshPreview) {
        m_refreshPreview = false;

        m_mutex.unlock();
        Mlt::Producer newProducer = getSlideshow();
        m_mutex.lock();

        if (!m_refreshPreview) {
            m_previewProducer = newProducer;
            QMetaObject::invokeMethod(this, "startPreview", Qt::QueuedConnection);
        }
    }
    m_mutex.unlock();
}

void SlideshowGeneratorWidget::startPreview()
{
    m_mutex.lock();
    if (m_previewProducer.is_valid()) {
        m_preview->start(m_previewProducer);
    }
    m_previewProducer = Mlt::Producer();
    m_mutex.unlock();
}
