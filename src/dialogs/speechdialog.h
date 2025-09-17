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

#ifndef SPEECHDIALOG_H
#define SPEECHDIALOG_H

#include <MltConsumer.h>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QObject>

namespace Mlt {
class Consumer;
}

class SpeechDialog : public QDialog
{
public:
    explicit SpeechDialog(QWidget *parent);
    QString outputFile() const { return m_outputFile ? m_outputFile->text().trimmed() : QString(); }
    QString languageCode() const
    {
        return m_language ? m_language->currentData().toString() : QString();
    }
    QString voiceCode() const { return m_voice ? m_voice->currentData().toString() : QString(); }
    double speed() const { return m_speed ? m_speed->value() : 1.0; }

private:
    QComboBox *m_language = nullptr;
    QComboBox *m_voice = nullptr;
    QDoubleSpinBox *m_speed = nullptr;
    QLineEdit *m_outputFile = nullptr;
    std::unique_ptr<Mlt::Consumer> m_consumer;
    void populateVoices(const QString &langCode);
};

#endif // SPEECHDIALOG_H
