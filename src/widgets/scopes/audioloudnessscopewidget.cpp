/*
 * Copyright (c) 2016 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#include "audioloudnessscopewidget.h"
#include <QDebug>
#include <QVBoxLayout>
#include <QQmlEngine>
#include <QDir>
#include <QQuickWidget>
#include <QQuickItem>
#include <QPushButton>
#include <QLabel>
#include <MltProfile.h>
#include "qmltypes/qmlutilities.h"
#include "mltcontroller.h"

AudioLoudnessScopeWidget::AudioLoudnessScopeWidget()
  : ScopeWidget("AudioLoudnessMeter")
  , m_loudnessFilter(0)
  , m_msElapsed(0)
  , m_orientation((Qt::Orientation)-1)
  , m_qview(new QQuickWidget(QmlUtilities::sharedEngine(), this))
  , m_timeLabel(new QLabel(this))
{
    qDebug() << "begin";
    m_loudnessFilter = new Mlt::Filter(MLT.profile(), "loudness_meter");
    setAutoFillBackground(true);

    m_qview->setFocusPolicy(Qt::StrongFocus);

    QmlUtilities::setCommonProperties(m_qview->rootContext());

    QVBoxLayout* vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(4, 4, 4, 4);
    vlayout->addWidget(m_qview);

    QHBoxLayout* hlayout = new QHBoxLayout();
    vlayout->addLayout(hlayout);

    // Add reset button
    QPushButton* button = new QPushButton(tr("Reset"), this);
    button->setToolTip(tr("Reset the measurement."));
    button->setCheckable(false);
    button->setMaximumWidth(100);
    hlayout->addWidget(button);
    connect(button, SIGNAL(clicked()), this, SLOT(onResetButtonClicked()));

    // Add time label
    m_timeLabel->setToolTip(tr("Time Since Reset"));
    m_timeLabel->setText("00:00:00:00");
    m_timeLabel->setFixedSize(this->fontMetrics().width("HH:MM:SS:MM"), this->fontMetrics().height());
    hlayout->addWidget(m_timeLabel);

    hlayout->addStretch();

    resetQview();

    qDebug() << "end";
}

AudioLoudnessScopeWidget::~AudioLoudnessScopeWidget()
{
    delete m_loudnessFilter;
}

void AudioLoudnessScopeWidget::refreshScope(const QSize& /*size*/, bool /*full*/)
{
    SharedFrame sFrame;
    while (m_queue.count() > 0) {
        sFrame = m_queue.pop();
        if (sFrame.is_valid() && sFrame.get_audio_samples() > 0) {
            mlt_audio_format format = mlt_audio_f32le;
            int channels = sFrame.get_audio_channels();
            int frequency = sFrame.get_audio_frequency();
            int samples = sFrame.get_audio_samples();
            Mlt::Frame mFrame = sFrame.clone(true, false, false);
            m_loudnessFilter->process(mFrame);
            mFrame.get_audio(format, frequency, channels, samples);
            m_msElapsed += (samples * 1000) / frequency;
        }
    }

    // Update the display every 100ms
    if (m_msElapsed >= 100) {
        m_msElapsed = 0;
        QMetaObject::invokeMethod(m_qview->rootObject(), "setValues", Qt::QueuedConnection,
            Q_ARG(QVariant, m_loudnessFilter->get_double("program")),
            Q_ARG(QVariant, m_loudnessFilter->get_double("shortterm")),
            Q_ARG(QVariant, m_loudnessFilter->get_double("momentary")),
            Q_ARG(QVariant, m_loudnessFilter->get_double("range")) );
    }

    // Update the time with every frame.
    m_timeLabel->setText( m_loudnessFilter->get_time( "frames_processed" ) );
}

QString AudioLoudnessScopeWidget::getTitle()
{
   return tr("Audio Loudness");
}

void AudioLoudnessScopeWidget::setOrientation(Qt::Orientation orientation)
{
    if (orientation != m_orientation) {
        if (orientation == Qt::Vertical) {
            setMinimumSize(220, 250);
            setMaximumSize(220, 500);
        } else {
            setMinimumSize(250, 210);
            setMaximumSize(500, 210);
        }
        updateGeometry();
        m_orientation = orientation;
    }
}

void AudioLoudnessScopeWidget::onResetButtonClicked()
{
	m_loudnessFilter->set("reset", 1);
    m_timeLabel->setText( "00:00:00:00" );
	resetQview();
}

bool AudioLoudnessScopeWidget::event(QEvent *event)
{
    bool result = ScopeWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        resetQview();
    }
    return result;
}

void AudioLoudnessScopeWidget::resetQview()
{
    qDebug();
    if (m_qview->status() != QQuickWidget::Null) {
        m_qview->setSource(QUrl(""));
    }

    QDir viewPath = QmlUtilities::qmlDir();
    viewPath.cd("scopes");
    viewPath.cd("audioloudness");
    m_qview->engine()->addImportPath(viewPath.path());

    QDir modulePath = QmlUtilities::qmlDir();
    modulePath.cd("modules");
    m_qview->engine()->addImportPath(modulePath.path());

    m_qview->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_qview->quickWindow()->setColor(palette().window().color());
    QUrl source = QUrl::fromLocalFile(viewPath.absoluteFilePath("audioloudnessscope.qml"));
    m_qview->setSource(source);
}
