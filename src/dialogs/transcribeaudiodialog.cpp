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

#include "transcribeaudiodialog.h"

#include "Logger.h"
#include "mainwindow.h"
#include "shotcut_mlt_properties.h"
#include "docks/timelinedock.h"

#include <MltProducer.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>

// List of supported languages from whispercpp
static const std::vector<const char *> whisperLanguages = {
    "en", "zh", "de", "es", "ru", "ko", "fr", "ja", "pt", "tr",
    "pl", "ca", "nl", "ar", "sv", "it", "id", "hi", "fi", "vi",
    "he", "uk", "el", "ms", "cs", "ro", "da", "hu", "ta", "no",
    "th", "ur", "hr", "bg", "lt", "la", "mi", "ml", "cy", "sk",
    "te", "fa", "lv", "bn", "sr", "az", "sl", "kn", "et", "mk",
    "br", "eu", "is", "hy", "ne", "mn", "bs", "kk", "sq", "sw",
    "gl", "mr", "pa", "si", "km", "sn", "yo", "so", "af", "oc",
    "ka", "be", "tg", "sd", "gu", "am", "yi", "lo", "uz", "fo",
    "ht", "ps", "tk", "nn", "mt", "sa", "lb", "my", "bo", "tl",
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
{
    setWindowTitle(tr("Transcribe Audio"));

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

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    grid->addWidget(m_buttonBox, 6, 0, 1, 2);
    connect(m_buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(clicked(QAbstractButton *)));

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

void TranscribeAudioDialog::clicked(QAbstractButton *button)
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
