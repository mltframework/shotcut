/*
 * Copyright (c) 2022 Meltytech, LLC
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

#include "alignaudiodialog.h"

#include "mainwindow.h"
#include "commands/timelinecommands.h"
#include "dialogs/alignmentarray.h"
#include "dialogs/longuitask.h"
#include "mltcontroller.h"
#include "proxymanager.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include "models/multitrackmodel.h"
#include "qmltypes/qmlapplication.h"
#include <Logger.h>

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTreeView>

class AudioReader : public QObject
{
    Q_OBJECT
public:
    AudioReader(QString producerXml, AlignmentArray* array, int in = -1, int out = -1)
        : QObject()
        , m_producerXml(producerXml)
        , m_array(array)
        , m_in(in)
        , m_out(out)
    {
    }

    void init(int maxLength)
    {
        m_array->init(maxLength);
    }

    void process()
    {
        QScopedPointer<Mlt::Producer> producer(new Mlt::Producer(MLT.profile(), "xml-string", m_producerXml.toUtf8().constData()));
        if (m_in >=0) {
            producer->set_in_and_out(m_in, m_out);
        }
        size_t frameCount = producer->get_playtime();
        std::vector<double> values(frameCount);
        int progress = 0;
        for (size_t i = 0; i < frameCount; ++i) {
            int frequency = 48000;
            int channels = 1;
            mlt_audio_format format = mlt_audio_s16;
            std::unique_ptr<Mlt::Frame> frame(producer->get_frame(i));
            mlt_position position = mlt_frame_get_position(frame->get_frame());
            int samples = mlt_audio_calculate_frame_samples(float(producer->get_fps()), frequency, position);
            int16_t* data = static_cast<int16_t*>(frame->get_audio(format, frequency, channels, samples));
            double sampleTotal = 0;
            // Add all values from the frame
            for (int k = 0; k < samples; ++k) {
                sampleTotal += data[k];
            }
            // Average the sample values
            values[i] = sampleTotal / samples;
            int newProgress = 100 * i / frameCount;
            if (newProgress != progress) {
                progress = newProgress;
                emit progressUpdate(progress);
            }
        }
        m_array->setValues(values);
    }

signals:
    void progressUpdate(int);

private:
    QString m_producerXml;
    AlignmentArray* m_array;
    int m_in;
    int m_out;
};

class ClipAudioReader : public QObject
{
    Q_OBJECT
public:
    ClipAudioReader(QString producerXml, AlignmentArray& referenceArray, int index, int in, int out)
        : QObject()
        , m_referenceArray(referenceArray)
        , m_reader(producerXml, &m_clipArray, in, out)
        , m_index(index)
        , m_calculateDrift(false)
    {
        connect(&m_reader, SIGNAL(progressUpdate(int)), this, SLOT(onReaderProgressUpdate(int)));
    }

    void init(int maxLength)
    {
        m_reader.init(maxLength);
    }

    void start()
    {
        m_future = QtConcurrent::run(this, &ClipAudioReader::process);
    }

    bool isFinished()
    {
        return m_future.isFinished();
    }

    void process()
    {
        onReaderProgressUpdate(0);
        m_reader.process();
        double drift = 1.0;
        int offset = 0;
        double quality;
        if (m_calculateDrift) {
            quality = m_referenceArray.calculateOffsetAndDrift(m_clipArray, 4, 0.1, &drift, &offset);
        } else {
            quality = m_referenceArray.calculateOffset(m_clipArray, &offset);
        }
        onReaderProgressUpdate(100);
        emit finished(m_index, offset, drift, quality);
    }

public slots:
    void onReaderProgressUpdate(int progress)
    {
        progress = progress * 99 / 100; // Reader goes from 0-99
        emit progressUpdate(m_index, progress);
    }

signals:
    void progressUpdate(int index, int percent);
    void finished(int index, int offset, double drift, double quality);

private:
    AlignmentArray m_clipArray;
    AlignmentArray& m_referenceArray;
    AudioReader m_reader;
    int m_index;
    QFuture<void> m_future;
    bool m_calculateDrift;
};

class AlignTableDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        const AlignClipsModel* model = dynamic_cast<const AlignClipsModel*>(index.model());
        switch (index.column()) {
        case AlignClipsModel::COLUMN_ERROR: {
            QIcon icon;
            if (!index.data().toString().isEmpty()) {
                icon = QIcon(":/icons/oxygen/32x32/status/task-reject.png");
            } else if (model->getProgress(index.row()) == 100) {
                icon = QIcon(":/icons/oxygen/32x32/status/task-complete.png");
            }
            icon.paint(painter, option.rect, Qt::AlignCenter);
            break;
        }
        case AlignClipsModel::COLUMN_NAME: {
            int progress = model->getProgress(index.row());
            if (progress > 0 ) {
                QStyleOptionProgressBar progressBarOption;
                progressBarOption.rect = option.rect;
                progressBarOption.minimum = 0;
                progressBarOption.maximum = 100;
                progressBarOption.progress = progress;
                QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
            }
            painter->drawText(option.rect, Qt::AlignLeft | Qt::AlignVCenter, index.data().toString() );
            break;
        }
        case AlignClipsModel::COLUMN_OFFSET:
//            case AlignClipsModel::COLUMN_DRIFT:
            QStyledItemDelegate::paint(painter, option, index);
            break;
        default:
            LOG_ERROR() << "Invalid Column" << index.row() << index.column();
            break;
        }
    }
};

// Include this so that AlignTableDelegate can be declared in the source file.
#include "alignaudiodialog.moc"

AlignAudioDialog::AlignAudioDialog(QString title, MultitrackModel* model, const QVector<QUuid>& uuids, QWidget* parent)
    : QDialog(parent)
    , m_model(model)
    , m_uuids(uuids)
    , m_uiTask(nullptr)
{
    int col = 0;
    setWindowTitle(title);
    setWindowModality(QmlApplication::dialogModality());

    QGridLayout* glayout = new QGridLayout();
    glayout->setHorizontalSpacing(4);
    glayout->setVerticalSpacing(2);
    // Track Combo
    glayout->addWidget(new QLabel(tr("Reference audio track")), col, 0, Qt::AlignRight);
    m_trackCombo = new QComboBox();
    int trackCount = m_model->trackList().size();
    for (int i = 0; i < trackCount; i++) {
        m_trackCombo->addItem(m_model->getTrackName(i), QVariant(i));
    }
    int defaultTrack = Settings.audioReferenceTrack();
    if (defaultTrack < trackCount) {
        m_trackCombo->setCurrentIndex(defaultTrack);
    }
    if (!connect(m_trackCombo, QOverload<int>::of(&QComboBox::activated), this, &AlignAudioDialog::rebuildClipList))
        connect(m_trackCombo, SIGNAL(activated(const QString&)), SLOT(rebuildClipList()));
    glayout->addWidget(m_trackCombo, col++, 1, Qt::AlignLeft);
    // List
    m_table = new QTreeView();
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setItemsExpandable(false);
    m_table->setRootIsDecorated(false);
    m_table->setUniformRowHeights(true);
    m_table->setSortingEnabled(false);
    m_table->setModel(&m_alignClipsModel);
    m_table->setWordWrap(false);
    m_delegate = new AlignTableDelegate();
    m_table->setItemDelegate(m_delegate);
    m_table->header()->setStretchLastSection(false);
    qreal rowHeight = fontMetrics().height() * devicePixelRatioF();
    m_table->header()->setMinimumSectionSize(rowHeight);
    m_table->header()->setSectionResizeMode(AlignClipsModel::COLUMN_ERROR, QHeaderView::Fixed);
    m_table->setColumnWidth(AlignClipsModel::COLUMN_ERROR, rowHeight);
    m_table->header()->setSectionResizeMode(AlignClipsModel::COLUMN_NAME, QHeaderView::Stretch);
    m_table->header()->setSectionResizeMode(AlignClipsModel::COLUMN_OFFSET, QHeaderView::Fixed);
    m_table->setColumnWidth(AlignClipsModel::COLUMN_OFFSET, fontMetrics().horizontalAdvance("-00:00:00:00") * devicePixelRatioF() + 8);
//    m_table->header()->setSectionResizeMode(AlignClipsModel::COLUMN_DRIFT, QHeaderView::ResizeToContents);
    glayout->addWidget(m_table, col++, 0, 1, 2);
    // Button Box + cancel
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    glayout->addWidget(m_buttonBox, col++, 0, 1, 2);
    // Process button
    QPushButton* processButton = m_buttonBox->addButton(tr("Process"), QDialogButtonBox::ActionRole);
    connect(processButton, SIGNAL(pressed()), this, SLOT(process()));
    // Apply button
    QPushButton* applyButton = m_buttonBox->addButton(tr("Apply"), QDialogButtonBox::ApplyRole);
    connect(applyButton, SIGNAL(pressed()), this, SLOT(apply()));
    // Process and apply button
    m_processAndApplyButton = m_buttonBox->addButton(tr("Process + Apply"), QDialogButtonBox::AcceptRole);
    connect(m_processAndApplyButton, SIGNAL(pressed()), this, SLOT(processAndApply()));

    this->setLayout (glayout);
    this->setModal(true);

    rebuildClipList();

    resize(500, 300);
}

AlignAudioDialog::~AlignAudioDialog()
{
    delete m_delegate;
    delete m_uiTask;
}

void AlignAudioDialog::rebuildClipList()
{
    QStringList stringList;
    m_alignClipsModel.clear();
    int referenceIndex = m_trackCombo->currentData().toInt();
    Settings.setAudioReferenceTrack(referenceIndex);

    for (const auto& uuid : m_uuids) {
        int trackIndex, clipIndex;
        QScopedPointer<Mlt::ClipInfo> info(m_model->findClipByUuid(uuid, trackIndex, clipIndex));
        if (info && info->cut && info->cut->is_valid()) {
            QString clipName = info->cut->get(kShotcutCaptionProperty);
            QString error;
            if (clipName.isNull()) {
                clipName = Util::baseName(ProxyManager::resource(*info->producer));
                if (clipName == "<producer>") {
                    clipName = QString::fromUtf8(info->cut->get("mlt_service"));
                }
            }
            if (trackIndex == referenceIndex) {
                error = tr("This clip will be skipped because it is on the reference track.");
            }
            m_alignClipsModel.addClip(clipName, AlignClipsModel::INVALID_OFFSET, AlignClipsModel::INVALID_OFFSET, error);
        }
    }
}

void AlignAudioDialog::process()
{
    m_uiTask = new LongUiTask(tr("Align Audio"));
    m_uiTask->setMinimumDuration(0);
    int referenceTrackIndex = m_trackCombo->currentData().toInt();
    auto mlt_index = m_model->trackList().at(referenceTrackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_model->tractor()->track(mlt_index));
    int maxLength = track->get_playtime();
    QString xml = MLT.XML(track.data());
    AlignmentArray trackArray;
    AudioReader trackReader(MLT.XML(track.data()), &trackArray);
    connect(&trackReader, SIGNAL(progressUpdate(int)), this, SLOT(updateReferenceProgress(int)));

    QList<ClipAudioReader*> m_clipReaders;
    for (const auto& uuid : m_uuids) {
        int trackIndex, clipIndex;
        QScopedPointer<Mlt::ClipInfo> info(m_model->findClipByUuid(uuid, trackIndex, clipIndex));
        if (!info || !info->cut || !info->cut->is_valid()) {
            continue;
        }
        if (trackIndex == referenceTrackIndex) {
            m_clipReaders.append(nullptr);
        } else {
            QString xml = MLT.XML(info->cut);
            ClipAudioReader* clipReader = new ClipAudioReader(xml, trackArray, m_clipReaders.size(), info->frame_in, info->frame_out);
            connect(clipReader, SIGNAL(progressUpdate(int, int)), this, SLOT(updateClipProgress(int, int)));
            connect(clipReader, SIGNAL(finished(int, int, double, double)), this, SLOT(clipFinished(int, int, double, double)));
            m_clipReaders.append(clipReader);
            maxLength = qMax(maxLength, info->frame_count);
        }
    }

    trackReader.init(maxLength);
    for (const auto& clipReader : m_clipReaders) {
        if (clipReader) clipReader->init(maxLength);
    }

    trackReader.process();
    for (const auto& clipReader : m_clipReaders) {
        if (clipReader) clipReader->start();
    }

    for (const auto& clipReader : m_clipReaders) {
        if (clipReader) {
            while (!clipReader->isFinished()) {
                QThread::msleep(10);
                QCoreApplication::processEvents();
            }
            clipReader->deleteLater();
        }
    }
    m_uiTask->deleteLater();
    m_uiTask = nullptr;
}

void AlignAudioDialog::apply()
{
    Timeline::AlignClipsCommand* command = new Timeline::AlignClipsCommand(*m_model);
    int referenceTrackIndex = m_trackCombo->currentData().toInt();
    int alignmentCount = 0;
    int modelIndex = 0;
    for (const auto& uuid : m_uuids) {
        int trackIndex, clipIndex;
        QScopedPointer<Mlt::ClipInfo> info(m_model->findClipByUuid(uuid, trackIndex, clipIndex));
        if (!info || !info->cut || !info->cut->is_valid()) {
            continue;
        }
        if (trackIndex != referenceTrackIndex) {
            int offset = m_alignClipsModel.getOffset(modelIndex);
            if (offset != AlignClipsModel::INVALID_OFFSET) {
                double speedCompensation = m_alignClipsModel.getDrift(modelIndex) * 100.0;
                command->addAlignment(uuid, offset, speedCompensation);
                alignmentCount++;
            }
        }
        modelIndex++;
    }
    if (alignmentCount > 0) {
        MAIN.undoStack()->push(command);
    } else {
        delete command;
    }
    accept();
}

void AlignAudioDialog::processAndApply()
{
    process();
    apply();
}

void AlignAudioDialog::updateReferenceProgress(int percent)
{
    if (m_uiTask) {
        m_uiTask->reportProgress(tr("Analyze Reference Track"), percent, 100);
    }
}

void AlignAudioDialog::updateClipProgress(int index, int percent)
{
    m_alignClipsModel.updateProgress(index, percent);
    if (m_uiTask) {
        m_uiTask->reportProgress(tr("Analyze Clips"), 0, 0);
    }
}

void AlignAudioDialog::clipFinished(int index, int offset, double drift, double quality)
{
    QString error;
    LOG_INFO() << "Clip" << index << "Offset:" << offset << "Quality:" << quality;
    if (quality < 0.01) {
        error = tr("Alignment not found.");
        offset = AlignClipsModel::INVALID_OFFSET;
        drift = AlignClipsModel::INVALID_OFFSET;
    }
    m_alignClipsModel.updateOffsetAndDrift(index, offset, drift, error);
    m_alignClipsModel.updateProgress(index, 100);
}
