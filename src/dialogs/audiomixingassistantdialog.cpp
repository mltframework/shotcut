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

#include "audiomixingassistantdialog.h"

#include "commands/filtercommands.h"
#include "commands/timelinecommands.h"
#include "controllers/filtercontroller.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/attachedfiltersmodel.h"
#include "models/multitrackmodel.h"
#include "qmltypes/qmlapplication.h"
#include "shotcut_mlt_properties.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFuture>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSet>
#include <QStackedWidget>
#include <QTableWidget>
#include <QThread>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include <cmath>

// How far below the program (dialogue anchor) loudness a frame must be to no
// longer count as "dialogue is speaking" for ducking purposes.
static const double kDuckingThresholdOffsetDb = -20.0;
static const int kMinDialogueEventMs = 500;
static const int kMaxDialogueGapMs = 1000;

// The dialogue compressor's threshold is set above the target program
// loudness so that normal speech passes through untouched.
static const double kCompressorThresholdOffsetDb = 8.0;
static const double kCompressorAttackMs = 15.0;
static const double kCompressorReleaseMs = 200.0;

// Below this, a track is already close enough to its target loudness that a
// normalization gain filter is not worth recommending.
static const double kGainNormalizationToleranceDb = 0.1;

static void removeAssistantFilters(Mlt::Producer &producer)
{
    for (int i = producer.filter_count() - 1; i >= 0; --i) {
        QScopedPointer<Mlt::Filter> filter(producer.filter(i));
        if (filter && filter->is_valid() && filter->get(kShotcutAudioMixingAssistantProperty)
            && strlen(filter->get(kShotcutAudioMixingAssistantProperty)) > 0)
            producer.detach(*filter);
    }
}

// Measures the integrated loudness of a track, and optionally a frame-by-frame
// level (for tracks that need to be protected, e.g. dialogue), in a worker thread.
class TrackAudioReader : public QObject
{
    Q_OBJECT
public:
    TrackAudioReader(int trackIndex, const QString &producerXml, bool measureFrameLevels)
        : m_trackIndex(trackIndex)
        , m_producerXml(producerXml)
        , m_measureFrameLevels(measureFrameLevels)
    {}

    void start() { m_future = QtConcurrent::run(&TrackAudioReader::process, this); }
    bool isFinished() { return m_future.isFinished(); }

    void process()
    {
        QScopedPointer<Mlt::Producer> producer(
            new Mlt::Producer(MLT.profile(), "xml-string", m_producerXml.toUtf8().constData()));
        if (!producer->is_valid()) {
            emit finished(m_trackIndex, -100.0, QVector<float>());
            return;
        }
        removeAssistantFilters(*producer);

        Mlt::Filter loudnessFilter(MLT.profile(), "loudness");
        // Without an explicit in/out, mlt_filter_get_length2() falls back to the
        // length of whichever individual clip produced the current frame (since
        // this filter is attached to the whole track, not a single clip), which
        // breaks the filter's end-of-analysis detection. Pin it to the track.
        int producerLength = producer->get_length();
        if (producerLength > 0)
            loudnessFilter.set_in_and_out(0, producerLength - 1);
        producer->attach(loudnessFilter);

        Mlt::Filter channels(MLT.profile(), "audiochannels");
        Mlt::Filter converter(MLT.profile(), "audioconvert");
        Mlt::Filter levelFilter(MLT.profile(), "audiolevel");
        if (m_measureFrameLevels) {
            producer->attach(channels);
            producer->attach(converter);
            producer->attach(levelFilter);
        }

        QVector<float> frameLevels;
        int n = producer->get_playtime();
        int progress = 0;
        for (int i = 0; i < n; i++) {
            QScopedPointer<Mlt::Frame> frame(producer->get_frame());
            if (frame && frame->is_valid()) {
                // Always pull audio, even for silent/blank frames, because the
                // loudness filter requires an unbroken sequence of frames.
                mlt_audio_format format = mlt_audio_f32le;
                int frequency = 48000;
                int channelCount = 2;
                int samples = mlt_audio_calculate_frame_samples(producer->get_fps(), frequency, i);
                frame->get_audio(format, frequency, channelCount, samples);
                if (m_measureFrameLevels) {
                    double left = frame->get_double("meta.media.audio_level.0");
                    double right = frame->get_double("meta.media.audio_level.1");
                    frameLevels.append(static_cast<float>(qMax(left, right)));
                }
            } else if (m_measureFrameLevels && !frameLevels.isEmpty()) {
                frameLevels.append(frameLevels.back());
            }

            int newProgress = n > 0 ? 100 * (i + 1) / n : 100;
            if (newProgress != progress) {
                progress = newProgress;
                emit progressUpdate(m_trackIndex, progress);
            }
        }

        double integratedLoudness = -100.0;
        QString results = QString::fromUtf8(loudnessFilter.get("results"));
        if (!results.isEmpty()) {
            QString token = results.split('\t').value(0); // "L: <value>"
            int colon = token.indexOf(':');
            if (colon >= 0)
                integratedLoudness = token.mid(colon + 1).trimmed().toDouble();
        }

        emit finished(m_trackIndex, integratedLoudness, frameLevels);
    }

signals:
    void progressUpdate(int trackIndex, int percent);
    void finished(int trackIndex, double integratedLoudness, QVector<float> frameLevels);

private:
    int m_trackIndex;
    QString m_producerXml;
    bool m_measureFrameLevels;
    QFuture<void> m_future;
};

AudioMixingAssistantDialog::AudioMixingAssistantDialog(QString title,
                                                       MultitrackModel *model,
                                                       QWidget *parent)
    : QDialog(parent)
    , m_model(model)
    , m_stage(StageConfigure)
    , m_blockUpdates(false)
    , m_pendingAnalysisCount(0)
{
    setWindowTitle(title);
    setWindowModality(QmlApplication::dialogModality());

    QVBoxLayout *vlayout = new QVBoxLayout();

    m_stack = new QStackedWidget();
    m_stack->addWidget(createConfigurePage());
    m_stack->addWidget(createProgressPage());
    m_stack->addWidget(createRecommendationsPage());
    m_stack->setCurrentIndex(StageConfigure);
    vlayout->addWidget(m_stack, 1);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_actionButton = m_buttonBox->addButton(tr("Process"), QDialogButtonBox::AcceptRole);
    connect(m_actionButton,
            &QPushButton::pressed,
            this,
            &AudioMixingAssistantDialog::onActionButtonPressed);
    vlayout->addWidget(m_buttonBox);

    setLayout(vlayout);
    setModal(true);

    m_blockUpdates = true;
    double targetLoudness = m_model ? m_model->getProjectTargetLoudness() : -14.0;
    m_targetLoudnessSpinner->setValue(targetLoudness);
    updateTargetPresetFromLoudness(targetLoudness);
    m_blockUpdates = false;

    populateTrackTable();

    resize(600, 400);
}

QWidget *AudioMixingAssistantDialog::createConfigurePage()
{
    QWidget *page = new QWidget();
    QVBoxLayout *vlayout = new QVBoxLayout(page);
    vlayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *targetLayout = new QHBoxLayout();
    targetLayout->addWidget(new QLabel(tr("Target Master Output")));
    m_targetPresetCombo = new QComboBox();
    m_targetPresetCombo->addItem(tr("Web/YouTube (-14 LUFS)"));
    m_targetPresetCombo->addItem(tr("Broadcast (-24 LUFS)"));
    m_targetPresetCombo->addItem(QString());
    targetLayout->addWidget(m_targetPresetCombo);
    m_targetLoudnessSpinner = new QDoubleSpinBox();
    m_targetLoudnessSpinner->setDecimals(1);
    m_targetLoudnessSpinner->setRange(-50.0, -5.0);
    m_targetLoudnessSpinner->setSingleStep(1.0);
    m_targetLoudnessSpinner->setSuffix(tr(" LUFS"));
    m_targetLoudnessSpinner->setToolTip(tr("The target integrated loudness of the master output."));
    targetLayout->addWidget(m_targetLoudnessSpinner);
    targetLayout->addStretch(1);
    vlayout->addLayout(targetLayout);

    QLabel *introLabel = new QLabel(
        tr("Assign a role to each track so the assistant knows what to protect and what to "
           "level. You can rename tracks here too."));
    introLabel->setWordWrap(true);
    vlayout->addWidget(introLabel);

    m_table = new QTableWidget();
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("Type"), tr("Track Name"), tr("Role")});
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(ColumnType, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColumnName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColumnRole, QHeaderView::ResizeToContents);
    connect(m_table,
            &QTableWidget::itemChanged,
            this,
            &AudioMixingAssistantDialog::onTrackNameChanged);
    vlayout->addWidget(m_table, 1);

    connect(m_targetPresetCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &AudioMixingAssistantDialog::onTargetPresetChanged);
    connect(m_targetLoudnessSpinner,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &AudioMixingAssistantDialog::onTargetLoudnessChanged);

    return page;
}

QWidget *AudioMixingAssistantDialog::createProgressPage()
{
    QWidget *page = new QWidget();
    QVBoxLayout *vlayout = new QVBoxLayout(page);

    vlayout->addStretch(1);
    QLabel *label = new QLabel(tr("Analyzing tracks..."));
    label->setAlignment(Qt::AlignHCenter);
    vlayout->addWidget(label);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    vlayout->addWidget(m_progressBar);
    vlayout->addStretch(1);

    return page;
}

QWidget *AudioMixingAssistantDialog::createRecommendationsPage()
{
    QWidget *page = new QWidget();
    QVBoxLayout *vlayout = new QVBoxLayout(page);
    vlayout->setContentsMargins(0, 0, 0, 0);

    QLabel *label = new QLabel(
        tr("Review the proposed changes below. Uncheck any item you do not want applied."));
    label->setWordWrap(true);
    vlayout->addWidget(label);

    m_recommendationsTable = new QTableWidget();
    m_recommendationsTable->setColumnCount(3);
    m_recommendationsTable->setHorizontalHeaderLabels(
        {tr("Apply"), tr("Track"), tr("Recommendation")});
    m_recommendationsTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_recommendationsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recommendationsTable->setWordWrap(true);
    m_recommendationsTable->verticalHeader()->setVisible(false);
    m_recommendationsTable->horizontalHeader()->setSectionResizeMode(RecommendationColumnEnabled,
                                                                     QHeaderView::ResizeToContents);
    m_recommendationsTable->horizontalHeader()->setSectionResizeMode(RecommendationColumnTrack,
                                                                     QHeaderView::ResizeToContents);
    m_recommendationsTable->horizontalHeader()->setSectionResizeMode(RecommendationColumnDescription,
                                                                     QHeaderView::Stretch);
    vlayout->addWidget(m_recommendationsTable, 1);

    return page;
}

QString AudioMixingAssistantDialog::roleDisplayName(AudioTrackRole role) const
{
    switch (role) {
    case DialogueAudioRole:
        return tr("Dialogue");
    case BackgroundMusicAudioRole:
        return tr("Background Music");
    case SoundEffectsAudioRole:
        return tr("Sound Effects");
    case AmbienceAudioRole:
        return tr("Ambience");
    case PremixedProgramAudioRole:
        return tr("Premixed Program");
    case UnassignedAudioRole:
    default:
        return tr("Unassigned");
    }
}

void AudioMixingAssistantDialog::populateTrackTable()
{
    if (!m_model)
        return;

    m_blockUpdates = true;
    m_table->setRowCount(0);
    int trackCount = m_model->trackList().size();
    m_table->setRowCount(trackCount);

    for (int row = 0; row < trackCount; row++) {
        bool isAudio = m_model->trackList().at(row).type == AudioTrackType;

        auto *typeItem = new QTableWidgetItem(isAudio ? tr("Audio") : tr("Video"));
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, ColumnType, typeItem);

        auto *nameItem = new QTableWidgetItem(m_model->getTrackName(row));
        nameItem->setData(Qt::UserRole, row);
        m_table->setItem(row, ColumnName, nameItem);

        auto *roleCombo = new QComboBox();
        roleCombo->addItem(roleDisplayName(UnassignedAudioRole));
        roleCombo->addItem(roleDisplayName(DialogueAudioRole));
        roleCombo->addItem(roleDisplayName(BackgroundMusicAudioRole));
        roleCombo->addItem(roleDisplayName(SoundEffectsAudioRole));
        roleCombo->addItem(roleDisplayName(AmbienceAudioRole));
        roleCombo->addItem(roleDisplayName(PremixedProgramAudioRole));
        roleCombo->setProperty("trackIndex", row);
        roleCombo->setCurrentIndex(static_cast<int>(m_model->getTrackRole(row)));
        connect(roleCombo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this,
                [this, row](int index) { onTrackRoleChanged(row, index); });
        m_table->setCellWidget(row, ColumnRole, roleCombo);
    }
    m_blockUpdates = false;
}

void AudioMixingAssistantDialog::onTrackNameChanged(QTableWidgetItem *item)
{
    if (m_blockUpdates || !item || item->column() != ColumnName)
        return;
    int trackIndex = item->data(Qt::UserRole).toInt();
    MAIN.undoStack()->push(new Timeline::NameTrackCommand(*m_model, trackIndex, item->text()));
}

void AudioMixingAssistantDialog::onTrackRoleChanged(int trackIndex, int roleComboIndex)
{
    if (m_blockUpdates || trackIndex < 0)
        return;
    MAIN.undoStack()->push(
        new Timeline::ChangeTrackRoleCommand(*m_model,
                                             trackIndex,
                                             static_cast<AudioTrackRole>(roleComboIndex)));
}

void AudioMixingAssistantDialog::updateTargetPresetFromLoudness(double value)
{
    bool wasBlocked = m_blockUpdates;
    m_blockUpdates = true;
    if (qFuzzyCompare(value, -14.0))
        m_targetPresetCombo->setCurrentIndex(PresetWeb);
    else if (qFuzzyCompare(value, -24.0))
        m_targetPresetCombo->setCurrentIndex(PresetBroadcast);
    else
        m_targetPresetCombo->setCurrentIndex(PresetCustom);
    m_blockUpdates = wasBlocked;
}

void AudioMixingAssistantDialog::onTargetPresetChanged(int index)
{
    if (m_blockUpdates)
        return;
    if (index == PresetWeb)
        m_targetLoudnessSpinner->setValue(-14.0);
    else if (index == PresetBroadcast)
        m_targetLoudnessSpinner->setValue(-24.0);
}

void AudioMixingAssistantDialog::onTargetLoudnessChanged(double value)
{
    updateTargetPresetFromLoudness(value);
    if (m_blockUpdates || !m_model)
        return;
    m_model->setProjectTargetLoudness(value);
}

void AudioMixingAssistantDialog::startProcessing()
{
    m_stage = StageProcessing;
    m_stack->setCurrentIndex(StageProcessing);
    m_actionButton->setEnabled(false);
    m_progressBar->setValue(0);

    m_analysisResults.clear();
    m_trackProgressByIndex.clear();

    QVector<TrackAudioReader *> readers;
    int trackCount = m_model->trackList().size();
    for (int row = 0; row < trackCount; row++) {
        AudioTrackRole role = m_model->getTrackRole(row);
        if (role == UnassignedAudioRole)
            continue;

        int mltIndex = m_model->trackList().at(row).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model->tractor()->track(mltIndex));
        if (!track || !track->is_valid())
            continue;

        bool measureFrameLevels = (role == DialogueAudioRole);
        auto *reader = new TrackAudioReader(row, MLT.XML(track.data()), measureFrameLevels);
        connect(reader,
                &TrackAudioReader::progressUpdate,
                this,
                &AudioMixingAssistantDialog::onTrackAnalysisProgress);
        connect(reader,
                &TrackAudioReader::finished,
                this,
                &AudioMixingAssistantDialog::onTrackAnalysisFinished);
        m_trackProgressByIndex[row] = 0;
        readers.append(reader);
    }

    m_pendingAnalysisCount = readers.size();
    if (readers.isEmpty()) {
        finishProcessing();
        return;
    }

    for (auto *reader : readers)
        reader->start();

    for (auto *reader : readers) {
        while (!reader->isFinished()) {
            QThread::msleep(10);
            QCoreApplication::processEvents();
        }
        reader->deleteLater();
    }
}

void AudioMixingAssistantDialog::onTrackAnalysisProgress(int trackIndex, int percent)
{
    m_trackProgressByIndex[trackIndex] = percent;
    int total = 0;
    for (int value : std::as_const(m_trackProgressByIndex))
        total += value;
    m_progressBar->setValue(total / m_trackProgressByIndex.size());
}

void AudioMixingAssistantDialog::onTrackAnalysisFinished(int trackIndex,
                                                         double integratedLoudness,
                                                         QVector<float> frameLevels)
{
    TrackAnalysisResult result;
    result.trackIndex = trackIndex;
    result.role = m_model->getTrackRole(trackIndex);
    result.integratedLoudness = integratedLoudness;
    result.frameLevels = frameLevels;
    m_analysisResults.append(result);

    if (--m_pendingAnalysisCount <= 0)
        finishProcessing();
}

void AudioMixingAssistantDialog::populateRecommendationsList()
{
    m_recommendationsTable->setRowCount(0);
    m_recommendationsTable->setRowCount(m_recommendations.size());
    for (int row = 0; row < m_recommendations.size(); row++) {
        const auto &recommendation = m_recommendations.at(row);

        auto *enabledItem = new QTableWidgetItem();
        enabledItem->setFlags((enabledItem->flags() & ~Qt::ItemIsEditable)
                              | Qt::ItemIsUserCheckable);
        enabledItem->setCheckState(recommendation.enabled ? Qt::Checked : Qt::Unchecked);
        m_recommendationsTable->setItem(row, RecommendationColumnEnabled, enabledItem);

        auto *trackItem = new QTableWidgetItem(m_model->getTrackName(recommendation.trackIndex));
        trackItem->setFlags(trackItem->flags() & ~Qt::ItemIsEditable);
        m_recommendationsTable->setItem(row, RecommendationColumnTrack, trackItem);

        auto *descriptionItem = new QTableWidgetItem(recommendation.description);
        descriptionItem->setFlags(descriptionItem->flags() & ~Qt::ItemIsEditable);
        m_recommendationsTable->setItem(row, RecommendationColumnDescription, descriptionItem);
    }
    m_recommendationsTable->resizeRowsToContents();
}

void AudioMixingAssistantDialog::addLoudnessRecommendation(const TrackAnalysisResult &result)
{
    double targetLoudness = m_model->getProjectTargetLoudness();
    if (std::abs(targetLoudness - result.integratedLoudness) < kGainNormalizationToleranceDb)
        return;
    double gain = qBound(-70.0, targetLoudness - result.integratedLoudness, 24.0);

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::DialogueLoudnessGain;
    recommendation.title = tr("Integrated Loudness Target");
    recommendation.description
        = tr("Add Gain/Volume filter: %1 dB (measured %2 LUFS, target %3 LUFS)")
              .arg(QString::number(gain, 'f', 1),
                   QString::number(result.integratedLoudness, 'f', 1),
                   QString::number(targetLoudness, 'f', 1));
    recommendation.mltService = "volume";
    recommendation.shotcutFilterName = "audioGain";
    recommendation.properties.append({"level", gain});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::addHighPassRecommendation(const TrackAnalysisResult &result)
{
    const double cutoffHz = 90.0; // between 80-100 Hz

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::DialogueHighPass;
    recommendation.title = tr("High-Pass Filter (Low Cut)");
    recommendation.description = tr("Add High Pass filter at %1 Hz to remove rumble, mic bumps, "
                                    "and wind noise")
                                     .arg(QString::number(cutoffHz, 'f', 0));
    recommendation.mltService = "ladspa.1890";
    recommendation.properties.append({"0", cutoffHz}); // cutoff frequency
    recommendation.properties.append({"1", 2.0});      // rolloff rate
    recommendation.properties.append({"wetness", 1.0});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::addCompressorRecommendation(const TrackAnalysisResult &result)
{
    const double ratio = 2.5;    // between 2:1 and 3:1
    const double kneeRadius = 6; // dB, soft knee
    double threshold = qBound(-30.0,
                              m_model->getProjectTargetLoudness() + kCompressorThresholdOffsetDb,
                              0.0);

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::DialogueCompressor;
    recommendation.title = tr("Dynamic Range Compression");
    recommendation.description = tr("Add Compressor: %1:1 ratio, soft knee, threshold %2 dB, to "
                                    "even out dialogue levels")
                                     .arg(QString::number(ratio, 'f', 1),
                                          QString::number(threshold, 'f', 1));
    recommendation.mltService = "ladspa.1882";
    recommendation.properties.append({"1", kCompressorAttackMs});
    recommendation.properties.append({"2", kCompressorReleaseMs});
    recommendation.properties.append({"3", threshold});
    recommendation.properties.append({"4", ratio});
    recommendation.properties.append({"5", kneeRadius});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::generateRecommendations()
{
    m_recommendations.clear();
    double anchor = dialogueAnchorLoudness();
    for (const auto &result : std::as_const(m_analysisResults)) {
        // -100 LUFS (or no measurement at all) means the track has no audible sound.
        if (result.integratedLoudness <= -100.0)
            continue;

        if (result.role == DialogueAudioRole) {
            addLoudnessRecommendation(result);
            addHighPassRecommendation(result);
            addCompressorRecommendation(result);
        } else if (result.role == BackgroundMusicAudioRole) {
            addBackgroundMusicGainRecommendation(result, anchor);
            addDuckingRecommendation(result);
            addFrequencyCarvingRecommendation(result);
        } else if (result.role == AmbienceAudioRole) {
            addAmbienceGainRecommendation(result, anchor);
            addAmbienceHighPassRecommendation(result);
            addAmbienceLowPassRecommendation(result);
        } else if (result.role == SoundEffectsAudioRole) {
            addSfxGainRecommendation(result, anchor);
            addSfxHighPassRecommendation(result);
            addSfxDuckingRecommendation(result);
        } else if (result.role == PremixedProgramAudioRole) {
            addPremixedProgramGainRecommendation(result);
        }
    }
}

double AudioMixingAssistantDialog::dialogueAnchorLoudness() const
{
    // Dialogue tracks are leveled to the project's target loudness, so that is
    // the anchor other roles (e.g. background music) are mixed relative to.
    return m_model ? m_model->getProjectTargetLoudness() : -14.0;
}

void AudioMixingAssistantDialog::addBackgroundMusicGainRecommendation(
    const TrackAnalysisResult &result, double anchorLoudness)
{
    const double offsetFromDialogue = -10.0;
    double targetLoudness = anchorLoudness + offsetFromDialogue;
    if (std::abs(targetLoudness - result.integratedLoudness) < kGainNormalizationToleranceDb)
        return;
    double gain = qBound(-70.0, targetLoudness - result.integratedLoudness, 24.0);

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::BackgroundMusicGain;
    recommendation.title = tr("Integrated Leveling Target (Base Gain)");
    recommendation.description = tr("Add Gain/Volume filter: %1 dB to sit %2 dB below the "
                                    "dialogue anchor (measured %3 LUFS, target %4 LUFS)")
                                     .arg(QString::number(gain, 'f', 1),
                                          QString::number(-offsetFromDialogue, 'f', 0),
                                          QString::number(result.integratedLoudness, 'f', 1),
                                          QString::number(targetLoudness, 'f', 1));
    recommendation.mltService = "volume";
    recommendation.shotcutFilterName = "audioGain";
    recommendation.properties.append({"level", gain});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::addDuckingRecommendation(const TrackAnalysisResult &result)
{
    const double attenuationMinDb = -6.0;
    const double attenuationMaxDb = -10.0;
    const double fadeDownMs = 300.0; // before speech starts
    const double fadeUpMs = 500.0;   // after speech finishes

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::BackgroundMusicDucking;
    recommendation.title = tr("Dynamic Ducking (Speech Overlap)");
    recommendation.description = tr("Add keyframed gain drops of %1 dB to %2 dB while dialogue is "
                                    "active, with a %3 ms fade-down before speech and a %4 ms "
                                    "fade-up after speech")
                                     .arg(QString::number(attenuationMinDb, 'f', 0),
                                          QString::number(attenuationMaxDb, 'f', 0),
                                          QString::number(fadeDownMs, 'f', 0),
                                          QString::number(fadeUpMs, 'f', 0));
    recommendation.mltService = "volume";
    recommendation.shotcutFilterName = "audioGain";
    recommendation.properties.append({"attenuationMinDb", attenuationMinDb});
    recommendation.properties.append({"attenuationMaxDb", attenuationMaxDb});
    recommendation.properties.append({"fadeDownMs", fadeDownMs});
    recommendation.properties.append({"fadeUpMs", fadeUpMs});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::addFrequencyCarvingRecommendation(const TrackAnalysisResult &result)
{
    const double dipFrequencyHz = 2000.0; // between 1 kHz and 3 kHz
    const double dipGainDb = -3.0;
    const double dipQ = 0.7; // broad notch

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::BackgroundMusicFrequencyCarving;
    recommendation.title = tr("Frequency Carving (Subtractive EQ)");
    recommendation.description = tr("Add Parametric EQ: %1 dB dip at %2 Hz to carve out space "
                                    "for vocal frequencies")
                                     .arg(QString::number(dipGainDb, 'f', 1),
                                          QString::number(dipFrequencyHz, 'f', 0));
    recommendation.mltService = "ladspa.1204";
    recommendation.shotcutFilterName = "parametricEq";
    // Low shelf and high shelf left flat; only the mid band (props 6-8) dips.
    recommendation.properties.append({"0", 0.0});
    recommendation.properties.append({"1", 50.0});
    recommendation.properties.append({"2", 0.5});
    recommendation.properties.append({"3", 0.0});
    recommendation.properties.append({"4", 100.0});
    recommendation.properties.append({"5", 1.0});
    recommendation.properties.append({"6", dipGainDb});
    recommendation.properties.append({"7", dipFrequencyHz});
    recommendation.properties.append({"8", dipQ});
    recommendation.properties.append({"9", 0.0});
    recommendation.properties.append({"10", 5000.0});
    recommendation.properties.append({"11", 1.0});
    recommendation.properties.append({"12", 0.0});
    recommendation.properties.append({"13", 15000.0});
    recommendation.properties.append({"14", 0.5});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::addAmbienceGainRecommendation(const TrackAnalysisResult &result,
                                                               double anchorLoudness)
{
    const double offsetFromDialogue = -16.0;
    double targetLoudness = anchorLoudness + offsetFromDialogue;
    if (std::abs(targetLoudness - result.integratedLoudness) < kGainNormalizationToleranceDb)
        return;
    double gain = qBound(-70.0, targetLoudness - result.integratedLoudness, 24.0);

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::AmbienceGain;
    recommendation.title = tr("Integrated Leveling Target (Base Gain)");
    recommendation.description = tr("Add Gain/Volume filter: %1 dB to sit %2 dB below the "
                                    "dialogue anchor (measured %3 LUFS, target %4 LUFS)")
                                     .arg(QString::number(gain, 'f', 1),
                                          QString::number(-offsetFromDialogue, 'f', 0),
                                          QString::number(result.integratedLoudness, 'f', 1),
                                          QString::number(targetLoudness, 'f', 1));
    recommendation.mltService = "volume";
    recommendation.shotcutFilterName = "audioGain";
    recommendation.properties.append({"level", gain});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::addAmbienceHighPassRecommendation(const TrackAnalysisResult &result)
{
    const double cutoffHz = 110.0; // between 100-120 Hz

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::AmbienceHighPass;
    recommendation.title = tr("Low-Frequency Filtering (High-Pass)");
    recommendation.description = tr("Add High Pass filter at %1 Hz to eliminate HVAC hum, "
                                    "traffic rumble, and wind turbulence")
                                     .arg(QString::number(cutoffHz, 'f', 0));
    recommendation.mltService = "ladspa.1890";
    recommendation.properties.append({"0", cutoffHz}); // cutoff frequency
    recommendation.properties.append({"1", 2.0});      // rolloff rate
    recommendation.properties.append({"wetness", 1.0});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::addAmbienceLowPassRecommendation(const TrackAnalysisResult &result)
{
    const double cutoffHz = 9000.0; // between 8-10 kHz

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::AmbienceLowPass;
    recommendation.title = tr("High-Frequency Filtering (Low-Pass)");
    recommendation.description = tr("Add Low Pass filter at %1 Hz to soften hiss, AC whine, and "
                                    "excessive room brightness")
                                     .arg(QString::number(cutoffHz, 'f', 0));
    recommendation.mltService = "ladspa.1891";
    recommendation.properties.append({"0", cutoffHz}); // cutoff frequency
    recommendation.properties.append({"1", 2.0});      // rolloff rate
    recommendation.properties.append({"wetness", 1.0});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::addSfxGainRecommendation(const TrackAnalysisResult &result,
                                                          double anchorLoudness)
{
    const double offsetFromDialogue = -6.0;
    double targetLoudness = anchorLoudness + offsetFromDialogue;
    if (std::abs(targetLoudness - result.integratedLoudness) < kGainNormalizationToleranceDb)
        return;
    double gain = qBound(-70.0, targetLoudness - result.integratedLoudness, 24.0);

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::SfxGain;
    recommendation.title = tr("Integrated Leveling Target (Base Gain)");
    recommendation.description = tr("Add Gain/Volume filter: %1 dB to sit %2 dB below the "
                                    "dialogue/program level (measured %3 LUFS, target %4 LUFS)")
                                     .arg(QString::number(gain, 'f', 1),
                                          QString::number(-offsetFromDialogue, 'f', 0),
                                          QString::number(result.integratedLoudness, 'f', 1),
                                          QString::number(targetLoudness, 'f', 1));
    recommendation.mltService = "volume";
    recommendation.shotcutFilterName = "audioGain";
    recommendation.properties.append({"level", gain});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::addSfxHighPassRecommendation(const TrackAnalysisResult &result)
{
    const double cutoffHz = 60.0;

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::SfxHighPass;
    recommendation.title = tr("Low-End Management");
    recommendation.description = tr("Add High Pass filter at %1 Hz to prevent low-frequency "
                                    "buildup")
                                     .arg(QString::number(cutoffHz, 'f', 0));
    recommendation.mltService = "ladspa.1890";
    recommendation.properties.append({"0", cutoffHz}); // cutoff frequency
    recommendation.properties.append({"1", 2.0});      // rolloff rate
    recommendation.properties.append({"wetness", 1.0});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::addSfxDuckingRecommendation(const TrackAnalysisResult &result)
{
    const double attenuationDb = -4.0;
    const double fadeDownMs = 300.0; // before speech starts
    const double fadeUpMs = 500.0;   // after speech finishes

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::SfxDucking;
    recommendation.title = tr("Dialogue Collision Clearance");
    recommendation.description = tr("Add keyframed gain drops of an additional %1 dB while "
                                    "dialogue is active to maintain clear vocal separation")
                                     .arg(QString::number(attenuationDb, 'f', 0));
    recommendation.mltService = "volume";
    recommendation.shotcutFilterName = "audioGain";
    recommendation.properties.append({"attenuationDb", attenuationDb});
    recommendation.properties.append({"fadeDownMs", fadeDownMs});
    recommendation.properties.append({"fadeUpMs", fadeUpMs});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::addPremixedProgramGainRecommendation(
    const TrackAnalysisResult &result)
{
    double targetLoudness = m_model->getProjectTargetLoudness();
    if (std::abs(targetLoudness - result.integratedLoudness) < kGainNormalizationToleranceDb)
        return;
    double gain = qBound(-70.0, targetLoudness - result.integratedLoudness, 24.0);

    MixRecommendation recommendation;
    recommendation.trackIndex = result.trackIndex;
    recommendation.kind = RecommendationKind::PremixedProgramGain;
    recommendation.title = tr("Integrated Loudness Target");
    recommendation.description = tr("Add Gain/Volume filter: %1 dB to reach the target program "
                                    "loudness (measured %2 LUFS, target %3 LUFS)")
                                     .arg(QString::number(gain, 'f', 1),
                                          QString::number(result.integratedLoudness, 'f', 1),
                                          QString::number(targetLoudness, 'f', 1));
    recommendation.mltService = "volume";
    recommendation.shotcutFilterName = "audioGain";
    recommendation.properties.append({"level", gain});
    m_recommendations.append(recommendation);
}

void AudioMixingAssistantDialog::finishProcessing()
{
    m_stage = StageRecommendations;
    generateRecommendations();
    populateRecommendationsList();
    m_stack->setCurrentIndex(StageRecommendations);
    m_recommendationsTable->resizeRowsToContents();
    m_actionButton->setText(tr("Apply"));
    m_actionButton->setEnabled(true);
}

QVector<QPair<int, int>> AudioMixingAssistantDialog::findDialogueEvents() const
{
    // A frame counts as "dialogue active" when any dialogue track's level
    // there is within kDuckingThresholdOffsetDb of the program loudness.
    double thresholdDb = dialogueAnchorLoudness() + kDuckingThresholdOffsetDb;

    int frameCount = 0;
    for (const auto &result : m_analysisResults)
        if (result.role == DialogueAudioRole)
            frameCount = qMax(frameCount, result.frameLevels.size());

    QVector<bool> active(frameCount, false);
    for (const auto &result : m_analysisResults) {
        if (result.role != DialogueAudioRole)
            continue;
        for (int i = 0; i < result.frameLevels.size(); i++) {
            double linear = result.frameLevels.at(i);
            double db = linear > 0.0 ? 20.0 * std::log10(linear) : -100.0;
            if (db >= thresholdDb)
                active[i] = true;
        }
    }

    double fps = MLT.profile().fps();
    int minEventFrames = qRound(kMinDialogueEventMs / 1000.0 * fps);
    int maxGapFrames = qRound(kMaxDialogueGapMs / 1000.0 * fps);

    // Find contiguous runs of active frames.
    QVector<QPair<int, int>> events;
    int start = -1;
    for (int i = 0; i < frameCount; i++) {
        if (active.at(i) && start < 0) {
            start = i;
        } else if (!active.at(i) && start >= 0) {
            events.append({start, i - 1});
            start = -1;
        }
    }
    if (start >= 0)
        events.append({start, frameCount - 1});

    // Ignore events that are too short to be a real dialogue utterance.
    QVector<QPair<int, int>> filtered;
    for (const auto &event : std::as_const(events)) {
        if (event.second - event.first + 1 >= minEventFrames)
            filtered.append(event);
    }

    // Combine events that are close enough together to duck as one.
    QVector<QPair<int, int>> merged;
    for (const auto &event : std::as_const(filtered)) {
        if (!merged.isEmpty() && event.first - merged.last().second - 1 < maxGapFrames)
            merged.last().second = event.second;
        else
            merged.append(event);
    }

    return merged;
}

QVector<DuckingKeyframe> AudioMixingAssistantDialog::generateDuckingKeyframes(
    const QVector<QPair<int, int>> &events,
    double maxGainDb,
    double duckedGainDb,
    int fadeDownFrames,
    int fadeUpFrames) const
{
    QVector<DuckingKeyframe> keyframes;
    for (const auto &event : events) {
        int dialogueStart = event.first;
        int dialogueEnd = event.second;

        // 1) Start of the fade-down, still at full volume.
        keyframes.append({qMax(dialogueStart - fadeDownFrames, 0), maxGainDb});
        // 2) Minimum (ducked) level, reached by the start of the dialogue.
        keyframes.append({dialogueStart, duckedGainDb});
        // 3) Start of the fade-up, at the end of the dialogue.
        keyframes.append({dialogueEnd, duckedGainDb});
        // 4) End of the fade-up, back to full volume.
        keyframes.append({dialogueEnd + fadeUpFrames, maxGainDb});
    }
    return keyframes;
}

// Looks up a named value from a recommendation's property list.
static double propertyValue(const QList<QPair<QString, double>> &properties,
                            const QString &name,
                            double defaultValue)
{
    for (const auto &property : properties) {
        if (property.first == name)
            return property.second;
    }
    return defaultValue;
}

static bool isGainKind(RecommendationKind kind)
{
    return kind == RecommendationKind::BackgroundMusicGain || kind == RecommendationKind::SfxGain;
}

static bool isDuckingKind(RecommendationKind kind)
{
    return kind == RecommendationKind::BackgroundMusicDucking
           || kind == RecommendationKind::SfxDucking;
}

static QString recommendationReason(RecommendationKind kind)
{
    switch (kind) {
    case RecommendationKind::DialogueLoudnessGain:
        return QObject::tr(
            "Gain adjustment to bring dialogue closer to the target program loudness.");
    case RecommendationKind::DialogueHighPass:
        return QObject::tr(
            "High-pass filter to reduce low-frequency rumble, mic bumps, and wind noise.");
    case RecommendationKind::DialogueCompressor:
        return QObject::tr(
            "Compressor to make dialogue volume more consistent and easier to hear.");
    case RecommendationKind::BackgroundMusicGain:
        return QObject::tr(
            "Gain adjustment to keep background music beneath dialogue and other critical audio.");
    case RecommendationKind::BackgroundMusicDucking:
        return QObject::tr("Ducking to lower background music while dialogue is active.");
    case RecommendationKind::BackgroundMusicFrequencyCarving:
        return QObject::tr("Parametric EQ to make space for dialogue in the vocal mid-range.");
    case RecommendationKind::AmbienceGain:
        return QObject::tr("Gain adjustment to keep ambience subtle beneath the dialogue.");
    case RecommendationKind::AmbienceHighPass:
        return QObject::tr("High-pass filter to reduce HVAC hum, traffic rumble, and wind noise.");
    case RecommendationKind::AmbienceLowPass:
        return QObject::tr("Low-pass filter to soften hiss, air-conditioner whine, and excessive "
                           "room brightness.");
    case RecommendationKind::SfxGain:
        return QObject::tr(
            "Gain adjustment to keep sound effects impactful without overpowering dialogue.");
    case RecommendationKind::SfxHighPass:
        return QObject::tr("High-pass filter to prevent low-frequency buildup in sound effects.");
    case RecommendationKind::SfxDucking:
        return QObject::tr(
            "Ducking to create clearer separation between sound effects and dialogue.");
    case RecommendationKind::PremixedProgramGain:
        return QObject::tr(
            "Gain adjustment to bring the premixed program closer to the target loudness.");
    case RecommendationKind::Unknown:
    default:
        return QString();
    }
}

void AudioMixingAssistantDialog::applyApprovedRecommendations()
{
    if (!m_model)
        return;

    QList<int> rowsToApply;
    for (int row = 0; row < m_recommendations.size(); row++) {
        if (m_recommendations.at(row).kind == RecommendationKind::Unknown)
            continue;
        QTableWidgetItem *enabledItem = m_recommendationsTable->item(row,
                                                                     RecommendationColumnEnabled);
        if (enabledItem && enabledItem->checkState() == Qt::Checked)
            rowsToApply.append(row);
    }
    if (rowsToApply.isEmpty())
        return;

    // When both a base gain and a ducking recommendation are approved for the
    // same track, they should become one "volume" filter, not two.
    QHash<int, int> gainRowByTrack;
    for (int row : std::as_const(rowsToApply)) {
        const auto &recommendation = m_recommendations.at(row);
        if (isGainKind(recommendation.kind))
            gainRowByTrack[recommendation.trackIndex] = row;
    }
    QSet<int> consumedRows;
    for (int row : std::as_const(rowsToApply)) {
        const auto &recommendation = m_recommendations.at(row);
        if (isDuckingKind(recommendation.kind) && gainRowByTrack.contains(recommendation.trackIndex))
            consumedRows.insert(gainRowByTrack.value(recommendation.trackIndex));
    }

    AttachedFiltersModel *attachedModel = MAIN.filterController()->attachedModel();
    QSet<int> tracksToReset;
    for (int row : std::as_const(rowsToApply))
        tracksToReset.insert(m_recommendations.at(row).trackIndex);

    MAIN.undoStack()->beginMacro(tr("Apply Audio Mixing Recommendations"));
    for (int trackIndex : std::as_const(tracksToReset)) {
        int mltIndex = m_model->trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model->tractor()->track(mltIndex));
        if (!track || !track->is_valid())
            continue;

        attachedModel->setProducer();
        attachedModel->setProducer(track.data());
        for (int filterRow = attachedModel->rowCount() - 1; filterRow >= 0; --filterRow) {
            QScopedPointer<Mlt::Service> service(attachedModel->getService(filterRow));
            if (service && service->is_valid() && service->get(kShotcutAudioMixingAssistantProperty)
                && strlen(service->get(kShotcutAudioMixingAssistantProperty)) > 0) {
                MAIN.undoStack()->push(new Filter::RemoveCommand(*attachedModel,
                                                                 attachedModel->name(filterRow),
                                                                 *service,
                                                                 filterRow));
            }
        }
    }
    // Dialogue events are the same for every ducking recommendation, so only compute them once.
    QVector<QPair<int, int>> dialogueEvents;
    bool haveDialogueEvents = false;

    for (int row : std::as_const(rowsToApply)) {
        if (consumedRows.contains(row))
            continue;
        const auto &recommendation = m_recommendations.at(row);
        int mltIndex = m_model->trackList().at(recommendation.trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model->tractor()->track(mltIndex));
        if (!track || !track->is_valid())
            continue;

        Mlt::Filter filter(MLT.profile(), recommendation.mltService.toUtf8().constData());
        if (!filter.is_valid())
            continue;
        if (!recommendation.shotcutFilterName.isEmpty())
            filter.set(kShotcutFilterProperty,
                       recommendation.shotcutFilterName.toUtf8().constData());
        filter.set(kShotcutAudioMixingAssistantProperty,
                   recommendationReason(recommendation.kind).toUtf8().constData());
        filter.set_in_and_out(track->get_in(), track->get_out());

        QString title = recommendation.title;
        if (isDuckingKind(recommendation.kind)) {
            if (!haveDialogueEvents) {
                dialogueEvents = findDialogueEvents();
                haveDialogueEvents = true;
            }
            int pairedGainRow = gainRowByTrack.value(recommendation.trackIndex, -1);
            double maxGainDb = pairedGainRow >= 0
                                   ? propertyValue(m_recommendations.at(pairedGainRow).properties,
                                                   "level",
                                                   0.0)
                                   : 0.0;
            if (pairedGainRow >= 0)
                title = tr("%1 + %2").arg(m_recommendations.at(pairedGainRow).title, title);

            double duckedGainDb;
            if (recommendation.kind == RecommendationKind::BackgroundMusicDucking) {
                double attenuationMinDb = propertyValue(recommendation.properties,
                                                        "attenuationMinDb",
                                                        -6.0);
                double attenuationMaxDb = propertyValue(recommendation.properties,
                                                        "attenuationMaxDb",
                                                        -10.0);
                duckedGainDb = maxGainDb + (attenuationMinDb + attenuationMaxDb) / 2.0;
            } else {
                duckedGainDb = maxGainDb
                               + propertyValue(recommendation.properties, "attenuationDb", -4.0);
            }
            double fps = MLT.profile().fps();
            int fadeDownFrames = qRound(
                propertyValue(recommendation.properties, "fadeDownMs", 300.0) / 1000.0 * fps);
            int fadeUpFrames = qRound(propertyValue(recommendation.properties, "fadeUpMs", 500.0)
                                      / 1000.0 * fps);

            auto keyframes = generateDuckingKeyframes(dialogueEvents,
                                                      maxGainDb,
                                                      duckedGainDb,
                                                      fadeDownFrames,
                                                      fadeUpFrames);
            int duration = track->get_length();
            filter.set("level", maxGainDb);
            for (const auto &keyframe : std::as_const(keyframes)) {
                int clampedFrame = qBound(0, keyframe.frame, qMax(duration - 1, 0));
                filter.anim_set("level",
                                keyframe.gainDb,
                                clampedFrame,
                                duration,
                                mlt_keyframe_smooth_natural);
            }
        } else {
            for (const auto &property : recommendation.properties)
                filter.set(property.first.toUtf8().constData(), property.second);
        }

        attachedModel->setProducer();
        attachedModel->setProducer(track.data());
        MAIN.undoStack()->push(
            new Filter::AddCommand(*attachedModel, title, filter, attachedModel->rowCount()));
    }
    MAIN.undoStack()->endMacro();
}

void AudioMixingAssistantDialog::onActionButtonPressed()
{
    switch (m_stage) {
    case StageConfigure:
        startProcessing();
        break;
    case StageRecommendations:
        applyApprovedRecommendations();
        accept();
        break;
    case StageProcessing:
    default:
        break;
    }
}

#include "audiomixingassistantdialog.moc"
