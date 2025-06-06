/*
 * Copyright (c) 2024-2025 Meltytech, LLC
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

#include "transcribeaudiodialog.h"

#include "Logger.h"
#include "dialogs/filedownloaddialog.h"
#include "docks/timelinedock.h"
#include "mainwindow.h"
#include "models/extensionmodel.h"
#include "qmltypes/qmlapplication.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <MltProducer.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTreeView>

// List of supported languages from whispercpp
static const std::vector<const char *> whisperLanguages = {
    "en", "zh", "de", "es",  "ru", "ko", "fr", "ja", "pt", "tr",  "pl", "ca", "nl", "ar", "sv",
    "it", "id", "hi", "fi",  "vi", "he", "uk", "el", "ms", "cs",  "ro", "da", "hu", "ta", "no",
    "th", "ur", "hr", "bg",  "lt", "la", "mi", "ml", "cy", "sk",  "te", "fa", "lv", "bn", "sr",
    "az", "sl", "kn", "et",  "mk", "br", "eu", "is", "hy", "ne",  "mn", "bs", "kk", "sq", "sw",
    "gl", "mr", "pa", "si",  "km", "sn", "yo", "so", "af", "oc",  "ka", "be", "tg", "sd", "gu",
    "am", "yi", "lo", "uz",  "fo", "ht", "ps", "tk", "nn", "mt",  "sa", "lb", "my", "bo", "tl",
    "mg", "as", "tt", "haw", "ln", "ha", "ba", "jw", "su", "yue",
};

static void fillLanguages(QComboBox *combo)
{
    QMap<QString, QString> codeMap;
    for (int i = 0; i < whisperLanguages.size(); i++) {
        QString langCode = whisperLanguages[i];
        QLocale::Language lang = QLocale::codeToLanguage(langCode);
        if (lang == QLocale::AnyLanguage) {
            LOG_ERROR() << "Language not found" << langCode;
            continue;
        }
        QString langStr = QLocale::languageToString(lang);
        if (!langCode.isEmpty() && !langStr.isEmpty()) {
            codeMap.insert(langStr, langCode);
        }
    }
    for (auto it = codeMap.keyValueBegin(); it != codeMap.keyValueEnd(); ++it) {
        combo->addItem(it->first, it->second);
    }
}

TranscribeAudioDialog::TranscribeAudioDialog(const QString &trackName, QWidget *parent)
    : QDialog(parent)
    , m_model("whispermodel")
{
    setWindowTitle(tr("Speech to Text"));
    setWindowModality(QmlApplication::dialogModality());

    Mlt::Producer *multitrack = MAIN.multitrack();

    if (!multitrack || !multitrack->is_valid()) {
        LOG_ERROR() << "Invalid multitrack";
        return;
    }

    QGridLayout *grid = new QGridLayout();

    grid->addWidget(new QLabel(tr("Name")), 0, 0, Qt::AlignRight);
    m_name = new QLineEdit(this);
    m_name->setText(trackName);
    grid->addWidget(m_name, 0, 1);

    grid->addWidget(new QLabel(tr("Language")), 1, 0, Qt::AlignRight);
    m_lang = new QComboBox(this);
    fillLanguages(m_lang);
    // Try to set the default to the system language
    QString currentLangCode = QLocale::languageToCode(QLocale::system().language(),
                                                      QLocale::ISO639Part1);
    for (int i = 0; i < m_lang->count(); i++) {
        if (m_lang->itemData(i).toString() == currentLangCode) {
            m_lang->setCurrentIndex(i);
            break;
        }
    }
    // Fall back to English
    if (m_lang->currentIndex() == -1) {
        for (int i = 0; i < m_lang->count(); i++) {
            if (m_lang->itemData(i).toString() == "en") {
                m_lang->setCurrentIndex(i);
                break;
            }
        }
    }
    grid->addWidget(m_lang, 1, 1);

    m_translate = new QCheckBox(this);
    m_translate->setCheckState(Qt::Unchecked);
    grid->addWidget(m_translate, 2, 0, Qt::AlignRight);
    grid->addWidget(new QLabel(tr("Translate to English")), 2, 1, Qt::AlignLeft);

    grid->addWidget(new QLabel(tr("Maximum line length")), 3, 0, Qt::AlignRight);
    m_maxLength = new QSpinBox(this);
    m_maxLength->setRange(10, 100);
    m_maxLength->setValue(42);
    m_maxLength->setSuffix(" characters");
    grid->addWidget(m_maxLength, 3, 1);

    m_nonspoken = new QCheckBox(this);
    m_nonspoken->setCheckState(Qt::Unchecked);
    grid->addWidget(m_nonspoken, 4, 0, Qt::AlignRight);
    grid->addWidget(new QLabel(tr("Include non-spoken sounds")), 4, 1, Qt::AlignLeft);

    QLabel *tracksLabel = new QLabel(tr("Tracks with speech"));
    tracksLabel->setToolTip(tr("Select tracks that contain speech to be transcribed."));
    grid->addWidget(tracksLabel, 5, 0, Qt::AlignRight);
    m_trackList = new QListWidget(this);
    m_trackList->setSelectionMode(QAbstractItemView::NoSelection);
    m_trackList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
    m_trackList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_trackList->setToolTip(tracksLabel->toolTip());
    Mlt::Tractor tractor(*multitrack);
    if (!tractor.is_valid()) {
        LOG_ERROR() << "Invalid tractor";
        return;
    }
    TrackList trackList = MAIN.timelineDock()->model()->trackList();
    if (trackList.size() == 0) {
        LOG_ERROR() << "No tracks";
        return;
    }
    for (int trackIndex = 0; trackIndex < trackList.size(); trackIndex++) {
        std::unique_ptr<Mlt::Producer> track(tractor.track(trackList[trackIndex].mlt_index));
        if (track) {
            QString trackName = QString::fromUtf8(track->get(kTrackNameProperty));
            if (!trackName.isEmpty()) {
                QListWidgetItem *listItem = new QListWidgetItem(trackName, m_trackList);
                if (track->get_int("hide") & 2) {
                    listItem->setCheckState(Qt::Unchecked);
                } else {
                    listItem->setCheckState(Qt::Checked);
                }
                listItem->setData(Qt::UserRole, QVariant(trackList[trackIndex].mlt_index));
                m_trackList->addItem(listItem);
            }
        }
    }
    grid->addWidget(m_trackList, 5, 1, Qt::AlignLeft);

    // The config section is a single widget with a unique grid layout inside of it.
    // The config section is hidden by hiding the config widget (and the layout it contains)
    static const int maxPathWidth = 350;
    m_configWidget = new QWidget(this);
    QGridLayout *configLayout = new QGridLayout(this);
    m_configWidget->setLayout(configLayout);

    // Horizontal separator line
    QFrame *line = new QFrame(m_configWidget);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    configLayout->addWidget(line, 0, 0, 1, 2);

    // Whisper.cpp exe
    configLayout->addWidget(new QLabel(tr("Whisper.cpp executable")), 1, 0, Qt::AlignRight);
    m_exeLabel = new QLineEdit(this);
    m_exeLabel->setFixedWidth(maxPathWidth);
    m_exeLabel->setReadOnly(true);
    configLayout->addWidget(m_exeLabel, 1, 1, Qt::AlignLeft);
    QPushButton *exeBrowseButton = new QPushButton(this);
    exeBrowseButton->setIcon(
        QIcon::fromTheme("document-open", QIcon(":/icons/oxygen/32x32/actions/document-open.png")));
    connect(exeBrowseButton, &QAbstractButton::clicked, this, [&] {
        auto path = QFileDialog::getOpenFileName(this,
                                                 tr("Find Whisper.cpp"),
                                                 Settings.whisperExe(),
                                                 QString(),
                                                 nullptr,
                                                 Util::getFileDialogOptions());
        if (QFileInfo(path).isExecutable()) {
            Settings.setWhisperExe(path);
            updateWhisperStatus();
        }
    });
    configLayout->addWidget(exeBrowseButton, 1, 2, Qt::AlignLeft);

    // Whisper.cpp model
    configLayout->addWidget(new QLabel(tr("GGML Model")), 2, 0, Qt::AlignRight);
    m_modelLabel = new QLineEdit(this);
    m_modelLabel->setFixedWidth(maxPathWidth);
    m_modelLabel->setPlaceholderText(tr("Select a model or browse to choose one"));
    m_modelLabel->setReadOnly(true);
    configLayout->addWidget(m_modelLabel, 2, 1, Qt::AlignLeft);
    QPushButton *modelBrowseButton = new QPushButton(this);
    modelBrowseButton->setIcon(
        QIcon::fromTheme("document-open", QIcon(":/icons/oxygen/32x32/actions/document-open.png")));
    connect(modelBrowseButton, &QAbstractButton::clicked, this, [&] {
        auto path = QFileDialog::getOpenFileName(this,
                                                 tr("Find Whisper.cpp"),
                                                 Settings.whisperModel(),
                                                 "*.bin",
                                                 nullptr,
                                                 Util::getFileDialogOptions());
        if (QFileInfo(path).exists()) {
            LOG_INFO() << "Model found" << path;
            Settings.setWhisperModel(path);
            updateWhisperStatus();
        } else {
            LOG_INFO() << "Model not found" << path;
        }
    });
    configLayout->addWidget(modelBrowseButton, 2, 2, Qt::AlignLeft);

    // List of models
    m_table = new QTreeView();
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setItemsExpandable(false);
    m_table->setRootIsDecorated(false);
    m_table->setUniformRowHeights(true);
    m_table->setSortingEnabled(false);
    m_table->setModel(&m_model);
    m_table->setWordWrap(false);
    m_table->header()->setStretchLastSection(false);
    qreal rowHeight = fontMetrics().height() * devicePixelRatioF();
    m_table->header()->setMinimumSectionSize(rowHeight);
    m_table->header()->setSectionResizeMode(ExtensionModel::COLUMN_STATUS, QHeaderView::Fixed);
    m_table->setColumnWidth(ExtensionModel::COLUMN_STATUS, rowHeight);
    m_table->header()->setSectionResizeMode(ExtensionModel::COLUMN_NAME, QHeaderView::Stretch);
    m_table->header()->setSectionResizeMode(ExtensionModel::COLUMN_SIZE, QHeaderView::Fixed);
    m_table->setColumnWidth(ExtensionModel::COLUMN_SIZE,
                            fontMetrics().horizontalAdvance("XXX.XX XXX") * devicePixelRatioF()
                                + 12);
    connect(m_table, &QAbstractItemView::clicked, this, &TranscribeAudioDialog::onModelRowClicked);

    configLayout->addWidget(m_table, 3, 0, 1, 3);

    grid->addWidget(m_configWidget, 6, 0, 1, 2);

    // Add a button box to the dialog
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *configButton = new QPushButton(tr("Configuration"));
    configButton->setCheckable(true);
    connect(configButton, &QPushButton::toggled, this, [&](bool checked) {
        m_configWidget->setVisible(checked);
    });
    updateWhisperStatus();
    QPushButton *okButton = m_buttonBox->button(QDialogButtonBox::Ok);
    if (!m_buttonBox->button(QDialogButtonBox::Ok)->isEnabled()) {
        // Show the config section
        configButton->setChecked(true);
        m_configWidget->setVisible(true);
    } else {
        configButton->setChecked(false);
        m_configWidget->setVisible(false);
    }
    m_buttonBox->addButton(configButton, QDialogButtonBox::ActionRole);
    grid->addWidget(m_buttonBox, 7, 0, 1, 2);
    connect(m_buttonBox,
            SIGNAL(clicked(QAbstractButton *)),
            this,
            SLOT(onButtonClicked(QAbstractButton *)));

    setLayout(grid);
    setModal(true);
    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

QList<int> TranscribeAudioDialog::tracks()
{
    QList<int> tracks;
    for (int i = 0; i < m_trackList->count(); i++) {
        QListWidgetItem *item = m_trackList->item(i);
        if (item && item->checkState() == Qt::Checked) {
            tracks << item->data(Qt::UserRole).toInt();
        }
    }
    return tracks;
}

void TranscribeAudioDialog::onButtonClicked(QAbstractButton *button)
{
    QDialogButtonBox::ButtonRole role = m_buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::AcceptRole) {
        LOG_DEBUG() << "Accept";
        accept();
    } else if (role == QDialogButtonBox::RejectRole) {
        LOG_DEBUG() << "Reject";
        reject();
    } else {
        LOG_DEBUG() << "Unknown role" << role;
    }
}

void TranscribeAudioDialog::onModelRowClicked(const QModelIndex &index)
{
    if (!m_model.downloaded(index.row())) {
        QMessageBox qDialog(QMessageBox::Question,
                            tr("Download Model"),
                            tr("Are you sure you want to download %1?\n%2 of storage will be used")
                                .arg(m_model.getName(index.row()))
                                .arg(m_model.getFormattedDataSize(index.row())),
                            QMessageBox::No | QMessageBox::Yes,
                            this);
        qDialog.setDefaultButton(QMessageBox::Yes);
        qDialog.setEscapeButton(QMessageBox::No);
        qDialog.setWindowModality(QmlApplication::dialogModality());
        int result = qDialog.exec();
        if (result == QMessageBox::Yes) {
            FileDownloadDialog dlDialog(tr("Download Model"), this);
            dlDialog.setSrc(m_model.url(index.row()));
            dlDialog.setDst(m_model.localPath(index.row()));
            dlDialog.start();
        }
    }

    if (m_model.downloaded(index.row())) {
        QString path = m_model.localPath(index.row());
        if (QFileInfo(path).exists()) {
            LOG_INFO() << "Model found" << path;
            Settings.setWhisperModel(path);
        } else {
            LOG_INFO() << "Model not found" << path;
        }
    }
    updateWhisperStatus();
}

QString TranscribeAudioDialog::name()
{
    return m_name->text();
}

QString TranscribeAudioDialog::language()
{
    return m_lang->currentData().toString();
}

bool TranscribeAudioDialog::translate()
{
    return m_translate->checkState() == Qt::Checked;
}

int TranscribeAudioDialog::maxLineLength()
{
    return m_maxLength->value();
}

bool TranscribeAudioDialog::includeNonspoken()
{
    return m_nonspoken->checkState() == Qt::Checked;
}

void TranscribeAudioDialog::updateWhisperStatus()
{
    bool exeFound = QFileInfo(Settings.whisperExe()).isExecutable();
    bool modelFound = QFileInfo(Settings.whisperModel()).exists();

    m_exeLabel->setText(Settings.whisperExe());
    m_modelLabel->setText(Settings.whisperModel());

    QPushButton *okButton = m_buttonBox->button(QDialogButtonBox::Ok);
    if (!exeFound || !modelFound) {
        // Disable the OK button;
        okButton->setDisabled(true);
    } else {
        okButton->setDisabled(false);
    }

    if (exeFound) {
        QPalette palette;
        m_exeLabel->setPalette(palette);
        m_exeLabel->setToolTip(tr("Path to Whisper.cpp executable"));
    } else {
        QPalette palette;
        palette.setColor(QPalette::Text, Qt::red);
        m_exeLabel->setPalette(palette);
        m_exeLabel->setToolTip(tr("Whisper.cpp executable not found"));
    }

    if (modelFound) {
        QPalette palette;
        m_modelLabel->setPalette(palette);
        m_modelLabel->setToolTip(tr("Path to GGML model"));
    } else {
        QPalette palette;
        palette.setColor(QPalette::Text, Qt::red);
        m_modelLabel->setPalette(palette);
        if (m_modelLabel->text().isEmpty()) {
            m_modelLabel->setText(m_modelLabel->placeholderText());
            m_modelLabel->setToolTip(tr("Select a model"));
        } else {
            m_modelLabel->setToolTip(tr("GGML model not found"));
        }
    }

    QModelIndex index = m_model.getIndexForPath(Settings.whisperModel());
    m_table->setCurrentIndex(index);
}
