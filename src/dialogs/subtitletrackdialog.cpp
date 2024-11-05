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

#include "subtitletrackdialog.h"

#include <QDebug>

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QLocale>
#include <QDialogButtonBox>
#include <QGridLayout>

static void fillLanguages(QComboBox *combo)
{
    QList<QLocale> allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript,
                                                         QLocale::AnyTerritory);
    QMap<QString, QString> iso639_2LanguageCodes;
    for (const QLocale &locale : allLocales) {
        QLocale::Language lang = locale.language();
        if (lang != QLocale::AnyLanguage && lang != QLocale::C) {
            QString langCode = QLocale::languageToCode(lang, QLocale::ISO639Part2);
            QString langStr = QLocale::languageToString(lang);
            if (!langCode.isEmpty() && !langStr.isEmpty()) {
                iso639_2LanguageCodes.insert(langStr, langCode);
            }
        }
    }
    for (auto it = iso639_2LanguageCodes.keyValueBegin(); it != iso639_2LanguageCodes.keyValueEnd();
            ++it) {
        QString text = QStringLiteral("%1 (%2)").arg(it->first).arg(it->second);
        combo->addItem(text, it->second);
    }
}

SubtitleTrackDialog::SubtitleTrackDialog(const QString &name, const QString &lang, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("New Subtitle Track"));

    QGridLayout *grid = new QGridLayout();

    grid->addWidget(new QLabel(tr("Name")), 0, 0, Qt::AlignRight);
    m_name = new QLineEdit(this);
    m_name->setText(name);
    grid->addWidget(m_name, 0, 1);

    grid->addWidget(new QLabel(tr("Language")), 1, 0, Qt::AlignRight);
    m_lang = new QComboBox(this);
    fillLanguages(m_lang);
    for (int i = 0; i < m_lang->count(); i++) {
        if (m_lang->itemData(i).toString() == lang) {
            m_lang->setCurrentIndex(i);
            break;
        }
    }
    grid->addWidget(m_lang, 1, 1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                       | QDialogButtonBox::Cancel);
    grid->addWidget(buttonBox, 2, 0, 2, 2);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    setLayout(grid);
    this->setModal(true);
    m_name->setFocus();
}

QString SubtitleTrackDialog::getName()
{
    return m_name->text();
}

QString SubtitleTrackDialog::getLanguage()
{
    return m_lang->currentData().toString();
}

void SubtitleTrackDialog::accept()
{
    QDialog::accept();
}
