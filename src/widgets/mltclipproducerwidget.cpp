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

#include "mltclipproducerwidget.h"

#include "Logger.h"
#include "mltcontroller.h"
#include "util.h"

#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

MltClipProducerWidget::MltClipProducerWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *vlayout = new QVBoxLayout();
    m_nameLabel = new QLabel();
    Util::setColorsToHighlight(m_nameLabel);
    QFont font = m_nameLabel->font();
    font.setBold(true);
    m_nameLabel->setFont(font);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    vlayout->addWidget(m_nameLabel);

    int row = 0;
    QGridLayout *glayout = new QGridLayout();
    glayout->setHorizontalSpacing(4);
    glayout->setVerticalSpacing(2);

    glayout->addWidget(new QLabel(tr("Resolution")), row, 0, Qt::AlignRight);
    glayout->addWidget(new QLabel(":"), row, 1, Qt::AlignLeft);
    m_resolutionLabel = new QLabel();
    glayout->addWidget(m_resolutionLabel, row, 2, Qt::AlignLeft);
    row++;

    glayout->addWidget(new QLabel(tr("Aspect ratio")), row, 0, Qt::AlignRight);
    glayout->addWidget(new QLabel(":"), row, 1, Qt::AlignLeft);
    m_aspectRatioLabel = new QLabel();
    glayout->addWidget(m_aspectRatioLabel, row, 2, Qt::AlignLeft);
    row++;

    glayout->addWidget(new QLabel(tr("Frame rate")), row, 0, Qt::AlignRight);
    glayout->addWidget(new QLabel(":"), row, 1, Qt::AlignLeft);
    m_frameRateLabel = new QLabel();
    glayout->addWidget(m_frameRateLabel, row, 2, Qt::AlignLeft);
    row++;

    glayout->addWidget(new QLabel(tr("Scan mode")), row, 0, Qt::AlignRight);
    glayout->addWidget(new QLabel(":"), row, 1, Qt::AlignLeft);
    m_scanModeLabel = new QLabel();
    glayout->addWidget(m_scanModeLabel, row, 2, Qt::AlignLeft);
    row++;

    glayout->addWidget(new QLabel(tr("Colorspace")), row, 0, Qt::AlignRight);
    glayout->addWidget(new QLabel(":"), row, 1, Qt::AlignLeft);
    m_colorspaceLabel = new QLabel();
    glayout->addWidget(m_colorspaceLabel, row, 2, Qt::AlignLeft);
    row++;

    auto spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    glayout->addWidget(spacer, row, 2);
    row++;

    vlayout->addLayout(glayout);

    this->setLayout (vlayout);
}

MltClipProducerWidget::~MltClipProducerWidget()
{
}

Mlt::Producer *MltClipProducerWidget::newProducer(Mlt::Profile &)
{
    // Not implemented
    return nullptr;
}

void MltClipProducerWidget::setProducer(Mlt::Producer *producer)
{
    m_producer = *producer;
    if (!m_producer.is_valid()) {
        m_nameLabel->setText("");
        m_nameLabel->setToolTip("");
        m_resolutionLabel->setText("");
        m_aspectRatioLabel->setText("");
        m_frameRateLabel->setText("");
        m_scanModeLabel->setText("");
        m_colorspaceLabel->setText("");
        return;
    }

    QString resource = Util::GetFilenameFromProducer(&m_producer);
    QString name = Util::baseName(resource, true);
    m_nameLabel->setText(name);
    m_nameLabel->setToolTip(resource);
    int width = m_producer.get_int("meta.media.width");
    int height = m_producer.get_int("meta.media.height");
    m_resolutionLabel->setText(QString("%1 x %2").arg(width).arg(height));
    int aspect_num = m_producer.get_int("meta.media.sample_aspect_num");
    int aspect_den = m_producer.get_int("meta.media.sample_aspect_den");
    m_aspectRatioLabel->setText(QString("%1 : %2").arg(aspect_num).arg(aspect_den));
    int frame_rate_num = m_producer.get_int("meta.media.frame_rate_num");
    int frame_rate_den = m_producer.get_int("meta.media.frame_rate_den");
    double fps = (double)frame_rate_num / (double)frame_rate_den;
    m_frameRateLabel->setText(tr("%L1 fps").arg(fps, 0, 'f', 6));
    if (m_producer.get_int("meta.media.progressive"))
        m_scanModeLabel->setText(tr("Progressive"));
    else
        m_scanModeLabel->setText(tr("Interlaced"));
    int colorspace = m_producer.get_int("meta.media.colorspace");
    if (colorspace == 601)
        m_colorspaceLabel->setText("ITU-R BT.601");
    else if (colorspace)
        m_colorspaceLabel->setText("ITU-R BT.709");
    else
        m_colorspaceLabel->setText("");
}
