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

#ifndef AUDIOMIXINGASSISTANTDIALOG_H
#define AUDIOMIXINGASSISTANTDIALOG_H

#include "models/multitrackmodel.h"

#include <QDialog>
#include <QHash>
#include <QVector>

class QTableWidget;
class QTableWidgetItem;
class QDialogButtonBox;
class QPushButton;
class QComboBox;
class QDoubleSpinBox;
class QStackedWidget;
class QProgressBar;
class TrackAudioReader;

// The measurements gathered for a single track during analysis.
struct TrackAnalysisResult
{
    int trackIndex = -1;
    AudioTrackRole role = UnassignedAudioRole;
    double integratedLoudness = -100.0;
    QVector<float> frameLevels; // only populated for dialogue tracks
};

// A single point in a keyframed gain envelope used to duck a track.
struct DuckingKeyframe
{
    int frame = 0;
    double gainDb = 0.0;
};

// Identifies what kind of change a recommendation represents, so the Apply
// step knows how to act on it.
enum class RecommendationKind {
    Unknown = 0,
    DialogueLoudnessGain,
    DialogueHighPass,
    DialogueCompressor,
    BackgroundMusicGain,
    BackgroundMusicDucking,
    BackgroundMusicFrequencyCarving,
    AmbienceGain,
    AmbienceHighPass,
    AmbienceLowPass,
    SfxGain,
    SfxHighPass,
    SfxDucking,
    PremixedProgramGain
};

// A single proposed filter to add to a track, pending user approval.
struct MixRecommendation
{
    int trackIndex = -1;
    RecommendationKind kind = RecommendationKind::Unknown;
    QString title;
    QString description;
    bool enabled = true;
    QString mltService;
    QString shotcutFilterName;
    QList<QPair<QString, double>> properties;
};

class AudioMixingAssistantDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AudioMixingAssistantDialog(QString title, MultitrackModel *model, QWidget *parent = 0);

private slots:
    void onTrackNameChanged(QTableWidgetItem *item);
    void onTrackRoleChanged(int trackIndex, int roleComboIndex);
    void onTargetPresetChanged(int index);
    void onTargetLoudnessChanged(double value);
    void onActionButtonPressed();
    void onTrackAnalysisProgress(int trackIndex, int percent);
    void onTrackAnalysisFinished(int trackIndex,
                                 double integratedLoudness,
                                 QVector<float> frameLevels);

private:
    enum Column { ColumnType = 0, ColumnName, ColumnRole };
    enum RecommendationColumn {
        RecommendationColumnEnabled = 0,
        RecommendationColumnTrack,
        RecommendationColumnDescription
    };
    enum TargetPreset { PresetWeb = 0, PresetBroadcast, PresetCustom };
    enum Stage { StageConfigure = 0, StageProcessing, StageRecommendations };

    QWidget *createConfigurePage();
    QWidget *createProgressPage();
    QWidget *createRecommendationsPage();
    void populateTrackTable();
    QString roleDisplayName(AudioTrackRole role) const;
    void updateTargetPresetFromLoudness(double value);
    void startProcessing();
    void finishProcessing();
    void generateRecommendations();
    void addLoudnessRecommendation(const TrackAnalysisResult &result);
    void addHighPassRecommendation(const TrackAnalysisResult &result);
    void addCompressorRecommendation(const TrackAnalysisResult &result);
    void addBackgroundMusicGainRecommendation(const TrackAnalysisResult &result,
                                              double anchorLoudness);
    void addDuckingRecommendation(const TrackAnalysisResult &result);
    void addFrequencyCarvingRecommendation(const TrackAnalysisResult &result);
    void addAmbienceGainRecommendation(const TrackAnalysisResult &result, double anchorLoudness);
    void addAmbienceHighPassRecommendation(const TrackAnalysisResult &result);
    void addAmbienceLowPassRecommendation(const TrackAnalysisResult &result);
    void addSfxGainRecommendation(const TrackAnalysisResult &result, double anchorLoudness);
    void addSfxHighPassRecommendation(const TrackAnalysisResult &result);
    void addSfxDuckingRecommendation(const TrackAnalysisResult &result);
    void addPremixedProgramGainRecommendation(const TrackAnalysisResult &result);
    double dialogueAnchorLoudness() const;
    void populateRecommendationsList();
    void applyApprovedRecommendations();
    QVector<QPair<int, int>> findDialogueEvents() const;
    QVector<DuckingKeyframe> generateDuckingKeyframes(const QVector<QPair<int, int>> &events,
                                                      double maxGainDb,
                                                      double duckedGainDb,
                                                      int fadeDownFrames,
                                                      int fadeUpFrames) const;

    MultitrackModel *m_model;
    QTableWidget *m_table;
    QComboBox *m_targetPresetCombo;
    QDoubleSpinBox *m_targetLoudnessSpinner;
    QStackedWidget *m_stack;
    QProgressBar *m_progressBar;
    QTableWidget *m_recommendationsTable;
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_actionButton;
    Stage m_stage;
    bool m_blockUpdates;
    QVector<TrackAnalysisResult> m_analysisResults;
    QVector<MixRecommendation> m_recommendations;
    QHash<int, int> m_trackProgressByIndex;
    int m_pendingAnalysisCount;
};

#endif // AUDIOMIXINGASSISTANTDIALOG_H
