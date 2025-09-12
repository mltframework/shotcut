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

#include "speechdialog.h"
#include "Logger.h"
#include "qmltypes/qmlapplication.h"
#include "settings.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>

SpeechDialog::SpeechDialog(QWidget *parent)
{
    setWindowTitle(tr("Text to Speech"));
    setWindowModality(QmlApplication::dialogModality());

    auto grid = new QGridLayout;
    setLayout(grid);
    grid->setSizeConstraint(QLayout::SetFixedSize);

    // Language selector row
    auto languageLabel = new QLabel(tr("Language"), this);
    m_language = new QComboBox(this);
    // value (userData) : visible name
    m_language->addItem(tr("American English"), QStringLiteral("a"));
    m_language->addItem(tr("British English"), QStringLiteral("b"));
    m_language->addItem(tr("Spanish"), QStringLiteral("e"));
    m_language->addItem(tr("French"), QStringLiteral("f"));
    m_language->addItem(tr("Hindi"), QStringLiteral("h"));
    m_language->addItem(tr("Italian"), QStringLiteral("i"));
    m_language->addItem(tr("Brazilian Portuguese"), QStringLiteral("p"));
    m_language->addItem(tr("Japanese"), QStringLiteral("j"));
    m_language->addItem(tr("Mandarin Chinese"), QStringLiteral("z"));
    // Set persisted language selection
    const QString savedLang = Settings.speechLanguage();
    for (int i = 0; i < m_language->count(); ++i) {
        if (m_language->itemData(i).toString() == savedLang) {
            m_language->setCurrentIndex(i);
            break;
        }
    }

    grid->addWidget(languageLabel, 0, 0, Qt::AlignRight);
    grid->addWidget(m_language, 0, 1);

    // Voice selector row
    auto voiceLabel = new QLabel(tr("Voice"), this);
    m_voice = new QComboBox(this);
    grid->addWidget(voiceLabel, 1, 0, Qt::AlignRight);
    grid->addWidget(m_voice, 1, 1);

    // Populate voices for initial selection and react to changes
    populateVoices(m_language->currentData().toString());
    connect(m_language, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        populateVoices(m_language->currentData().toString());
    });
    // Set persisted voice (after voices are populated again below if needed)
    const QString savedVoice = Settings.speechVoice();
    if (!savedVoice.isEmpty()) {
        for (int i = 0; i < m_voice->count(); ++i) {
            if (m_voice->itemData(i).toString() == savedVoice) {
                m_voice->setCurrentIndex(i);
                break;
            }
        }
    }

    // Speed selector row
    auto speedLabel = new QLabel(tr("Speed"), this);
    m_speed = new QDoubleSpinBox(this);
    m_speed->setRange(0.5, 2.0);
    m_speed->setDecimals(2);
    m_speed->setSingleStep(0.05);
    m_speed->setValue(Settings.speechSpeed());
    grid->addWidget(speedLabel, 2, 0, Qt::AlignRight);
    grid->addWidget(m_speed, 2, 1);

    // Output file row
    auto outputLabel = new QLabel(tr("Output file"), this);
    m_outputFile = new QLineEdit(this);
    m_outputFile->setMinimumWidth(300);
    m_outputFile->setPlaceholderText(tr("Click the button to set the file"));
    m_outputFile->setDisabled(true);
    m_outputFile->setText(QmlApplication::getNextProjectFile("speech-.wav"));
    auto icon = QIcon::fromTheme("document-save",
                                 QIcon(":/icons/oxygen/32x32/actions/document-save.png"));
    auto browseButton = new QPushButton(icon, QString(), this);
    grid->addWidget(outputLabel, 3, 0, Qt::AlignRight);
    auto outputRow = new QWidget(this);
    auto outputLayout = new QHBoxLayout(outputRow);
    outputLayout->setContentsMargins(0, 0, 0, 0);
    outputLayout->setSpacing(4);
    outputLayout->addWidget(m_outputFile);
    outputLayout->addWidget(browseButton);
    grid->addWidget(outputRow, 3, 1, 1, 2);

    connect(browseButton, &QPushButton::clicked, this, [this]() {
        const QString selected = QFileDialog::getSaveFileName(this,
                                                              tr("Save Audio File"),
                                                              Settings.savePath(),
                                                              tr("WAV files (*.wav)"));
        if (!selected.isEmpty()) {
            QString path = selected;
            if (!path.endsWith(QStringLiteral(".wav"), Qt::CaseInsensitive)) {
                path += QStringLiteral(".wav");
            }
            m_outputFile->setText(path);
            Settings.setSavePath(QFileInfo(selected).path());
        }
    });

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    grid->addWidget(buttonBox, 4, 0, 1, 3);
    connect(buttonBox->button(QDialogButtonBox::Cancel),
            &QAbstractButton::clicked,
            this,
            &QDialog::close);
    connect(buttonBox->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this, [&] {
        const QString lang = m_language->currentData().toString();
        const QString voice = m_voice->currentData().toString();
        const double speed = m_speed->value();

        QString path = m_outputFile->text().trimmed();
        if (path.isEmpty()) {
            LOG_DEBUG() << Settings.savePath();
            // No file chosen; prompt user similarly to previous behavior
            const QString selected = QFileDialog::getSaveFileName(this,
                                                                  tr("Save Audio File"),
                                                                  Settings.savePath(),
                                                                  tr("WAV files (*.wav)"));
            if (selected.isEmpty()) {
                return; // user canceled
            }
            path = selected;
            Settings.setSavePath(QFileInfo(selected).path());
        }
        if (!path.endsWith(QStringLiteral(".wav"), Qt::CaseInsensitive)) {
            path += QStringLiteral(".wav");
        }
        m_outputFile->setText(path);
        // Persist settings
        Settings.setSpeechLanguage(lang);
        Settings.setSpeechVoice(voice);
        Settings.setSpeechSpeed(speed);
        LOG_DEBUG() << "OK clicked, language code:" << lang << "voice:" << voice
                    << "speed:" << speed << "file:" << path;
        accept();
    });
}

void SpeechDialog::populateVoices(const QString &langCode)
{
    static const QStringList kVoices = {
        QStringLiteral("af_alloy"),    QStringLiteral("af_aoede"),
        QStringLiteral("af_bella"),    QStringLiteral("af_heart"),
        QStringLiteral("af_jessica"),  QStringLiteral("af_kore"),
        QStringLiteral("af_nicole"),   QStringLiteral("af_nova"),
        QStringLiteral("af_river"),    QStringLiteral("af_sarah"),
        QStringLiteral("af_sky"),      QStringLiteral("am_adam"),
        QStringLiteral("am_echo"),     QStringLiteral("am_eric"),
        QStringLiteral("am_fenrir"),   QStringLiteral("am_liam"),
        QStringLiteral("am_michael"),  QStringLiteral("am_onyx"),
        QStringLiteral("am_puck"),     QStringLiteral("am_santa"),
        QStringLiteral("bf_alice"),    QStringLiteral("bf_emma"),
        QStringLiteral("bf_isabella"), QStringLiteral("bf_lily"),
        QStringLiteral("bm_daniel"),   QStringLiteral("bm_fable"),
        QStringLiteral("bm_george"),   QStringLiteral("bm_lewis"),
        QStringLiteral("ef_dora"),     QStringLiteral("em_alex"),
        QStringLiteral("em_santa"),    QStringLiteral("ff_siwis"),
        QStringLiteral("hf_alpha"),    QStringLiteral("hf_beta"),
        QStringLiteral("hm_omega"),    QStringLiteral("hm_psi"),
        QStringLiteral("if_sara"),     QStringLiteral("im_nicola"),
        QStringLiteral("jf_alpha"),    QStringLiteral("jf_gongitsune"),
        QStringLiteral("jf_nezumi"),   QStringLiteral("jf_tebukuro"),
        QStringLiteral("jm_kumo"),     QStringLiteral("pf_dora"),
        QStringLiteral("pm_alex"),     QStringLiteral("pm_santa"),
        QStringLiteral("zf_xiaobei"),  QStringLiteral("zf_xiaoni"),
        QStringLiteral("zf_xiaoxiao"), QStringLiteral("zf_xiaoyi"),
        QStringLiteral("zm_yunjian"),  QStringLiteral("zm_yunxi"),
        QStringLiteral("zm_yunxia"),   QStringLiteral("zm_yunyang"),
    };

    m_voice->clear();
    if (langCode.isEmpty()) {
        return;
    }
    const QString prefix = langCode.left(1);
    for (const auto &v : kVoices) {
        if (v.startsWith(prefix)) {
            const int underscore = v.indexOf('_');
            if (underscore > 0 && underscore + 1 < v.size()) {
                const QString name = v.mid(underscore + 1);
                const QString gender = v.mid(1, 1).toLower();
                if (gender == "m") {
                    m_voice->addItem(QStringLiteral("♂️ ") + name[0].toUpper() + name.mid(1), v);
                } else if (gender == "f") {
                    m_voice->addItem(QStringLiteral("♀️ ") + name[0].toUpper() + name.mid(1), v);
                } else {
                    m_voice->addItem(name[0].toUpper() + name.mid(1), v);
                }
            }
        }
    }
    if (m_voice->count() == 0) {
        m_voice->addItem(tr("(No voices)"), QString());
    }
}
