/*
 * Copyright (c) 2024 Meltytech, LLC
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

#include "audiovectorscopewidget.h"

#include <Logger.h>
#include "settings.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <cmath>

static const qreal MAX_AMPLITUDE = 32768.0;

AudioVectorScopeWidget::AudioVectorScopeWidget()
    : ScopeWidget("AudioVector")
    , m_mutex()
    , m_c1Index(0)
    , m_c2Index(1)
{
    LOG_DEBUG() << "begin";
    setMinimumSize(100, 100);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);
    QHBoxLayout *hlayout = new QHBoxLayout(this);
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    vlayout->addLayout(hlayout);

    m_c1Combo = new QComboBox(this);
    m_c2Combo = new QComboBox(this);
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    hlayout->addWidget(m_c1Combo);
    hlayout->addWidget(spacer);
    hlayout->addWidget(m_c2Combo);

    m_imgLabel = new QLabel(this);
    m_imgLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_imgLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    vlayout->addWidget(m_imgLabel);

    setLayout(vlayout);

    connect(&Settings, &ShotcutSettings::playerAudioChannelsChanged, this, [&]() {
        setComboBoxOptions();
        requestRefresh();
    });
    connect(m_c1Combo, &QComboBox::currentIndexChanged, this, [&](int index) {
        m_c1Index = index;
        requestRefresh();
    });
    connect(m_c2Combo, &QComboBox::currentIndexChanged, this, [&](int index) {
        m_c2Index = index;
        requestRefresh();
    });
    setComboBoxOptions();

    LOG_DEBUG() << "end";
}

AudioVectorScopeWidget::~AudioVectorScopeWidget()
{
}

void AudioVectorScopeWidget::setComboBoxOptions()
{
    int channels = Settings.playerAudioChannels();
    m_c1Combo->clear();
    m_c2Combo->clear();
    if (channels == 1) {
        m_c1Combo->addItem(tr("C"));
        m_c2Combo->addItem(tr("C"));
    } else if (channels > 1) {
        m_c1Combo->addItem(tr("L"));
        m_c1Combo->addItem(tr("R"));
        m_c2Combo->addItem(tr("L"));
        m_c2Combo->addItem(tr("R"));
    }
    if (channels > 2 && channels != 4) {
        m_c1Combo->addItem(tr("C"));
        m_c2Combo->addItem(tr("C"));
    }
    if (channels > 3) {
        m_c1Combo->addItem(tr("Ls"));
        m_c1Combo->addItem(tr("Rs"));
        m_c2Combo->addItem(tr("Ls"));
        m_c2Combo->addItem(tr("Rs"));
    }
    if (channels == 6) {
        m_c1Combo->addItem(tr("LFE"));
        m_c2Combo->addItem(tr("LFE"));
    }
    m_c1Combo->setCurrentIndex(0);
    if (m_c2Combo->count() > 1) {
        m_c2Combo->setCurrentIndex(1);
    } else {
        m_c2Combo->setCurrentIndex(0);
    }
}

void AudioVectorScopeWidget::onNewDisplayImage()
{
    m_mutex.lock();
    QPixmap pixmap = QPixmap::fromImage(m_displayImg);
    m_mutex.unlock();
    pixmap = pixmap.scaled(m_imgLabel->width(), m_imgLabel->height(), Qt::KeepAspectRatio);
    m_imgLabel->setPixmap(pixmap);
}

void AudioVectorScopeWidget::refreshScope(const QSize &size, bool full)
{
    Q_UNUSED(full)

    while (m_queue.count() > 0) {
        m_frame = m_queue.pop();
    }

    qreal side = qMin(size.width(), size.height());

    if (m_renderImg.width() != side) {
        m_renderImg = QImage(QSize(side, side), QImage::Format_ARGB32_Premultiplied);
    }

    m_renderImg.fill(Qt::transparent);

    QPainter p(&m_renderImg);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Draw the diagonal axis
    QPen pen(Qt::DashLine);
    pen.setColor(palette().text().color().rgb());
    pen.setWidth(1);
    p.setPen(pen);
    p.drawLine(QPoint(0, 0), QPoint(side, side));
    p.drawLine(QPoint(0, side), QPoint(side, 0));

    if (m_frame.is_valid() && m_frame.get_audio_samples() > 0) {
        // Set up the painter for the vector points
        pen.setStyle(Qt::SolidLine);
        pen.setWidth(2);
        p.setPen(pen);
        // Add a transform to apply rotation
        QRectF rect = m_renderImg.rect();
        QPointF center = rect.center();
        QTransform t;
        t = t.translate(center.x(), center.y());
        t = t.rotate(45.0);
        p.setTransform(t);

        int channels = m_frame.get_audio_channels();
        int c1 = m_c1Index;
        int c2 = m_c2Index;
        if (c1 < 0 || c1 >= channels) {
            c1 = 0;
        }
        if (c2 < 0 || c2 >= channels) {
            c2 = 0;
        }

        int samples = m_frame.get_audio_samples();

        // Find the max value to be used for scaling
        const int16_t *a = (int16_t *)m_frame.get_audio();
        int16_t maxSampleValue = 0;
        for (int s = 0; s < samples; s++) {
            if (fabs(a[c1]) > maxSampleValue) {
                maxSampleValue = fabs(a[c1]);
            }
            if (fabs(a[c2]) > maxSampleValue) {
                maxSampleValue = fabs(a[c2]);
            }
            a += channels;
        }

        a = (int16_t *)m_frame.get_audio();
        qreal maxPoint = sqrt(side * side + side * side) / 4;
        qreal scaleFactor = maxPoint / maxSampleValue;
        for (int s = 0; s < samples; s++) {
            QPointF point((qreal)a[c1] * scaleFactor, (qreal)a[c2] * scaleFactor);
            p.drawPoint(point);
            a += channels;
        }
    }

    p.end();

    m_mutex.lock();
    m_displayImg.swap(m_renderImg);
    m_mutex.unlock();
    // Tell the GUI thread that a new image is ready.
    QMetaObject::invokeMethod(this, "onNewDisplayImage", Qt::QueuedConnection);
}

QString AudioVectorScopeWidget::getTitle()
{
    return tr("Audio Vector");
}
