/*
 * Copyright (c) 2022-2026 Meltytech, LLC
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

#include "notesdock.h"

#include "Logger.h"
#include "actions.h"
#include "dialogs/speechdialog.h"
#include "jobqueue.h"
#include "jobs/kokorodokijob.h"
#include "mltcontroller.h"
#include "settings.h"
#include "util.h"
#include "widgets/docktoolbar.h"

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

class TextEditor : public QPlainTextEdit
{
    Q_DECLARE_TR_FUNCTIONS(TextEditor)

public:
    explicit TextEditor(QWidget *parent = nullptr)
        : QPlainTextEdit()
    {
        setObjectName("Notes");
        zoomIn(Settings.notesZoom());
        setTabChangesFocus(false);
        setTabStopDistance(fontMetrics().horizontalAdvance("XXXX")); // Tabstop = 4 spaces
        setContextMenuPolicy(Qt::CustomContextMenu);

        auto icon = QIcon::fromTheme("zoom-out", QIcon(":/icons/oxygen/32x32/actions/zoom-out.png"));
        auto action = new QAction(icon, tr("Decrease Text Size"), this);
        action->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_Minus);
        addAction(action);
        Actions.add("notesDecreaseTextSize", action, objectName());
        connect(action, &QAction::triggered, this, [=]() { setZoom(-4); });

        icon = QIcon::fromTheme("zoom-in", QIcon(":/icons/oxygen/32x32/actions/zoom-in.png"));
        action = new QAction(icon, tr("Increase Text Size"), this);
        action->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_Equal);
        addAction(action);
        Actions.add("notesIncreaseTextSize", action, objectName());
        connect(action, &QAction::triggered, this, [=]() { setZoom(4); });

#ifdef EXTERNAL_LAUNCHERS
        icon = QIcon::fromTheme("text-speak", QIcon(":/icons/oxygen/32x32/actions/text-speak.png"));
        action = new QAction(icon, tr("Text to Speech..."), this);
        addAction(action);
        Actions.add("notesTextToSpeech", action, objectName());
        connect(action, &QAction::triggered, this, [=] {
            if (toPlainText().isEmpty() || !KokorodokiJob::checkDockerImage(this))
                return;

            m_speechDialog.reset(new SpeechDialog(this));
            if (m_speechDialog->exec() != QDialog::Accepted)
                return;

            KokorodokiJob::prepareAndRun(this, [=]() {
                const auto outFile = m_speechDialog->outputFile();
                if (outFile.isEmpty())
                    return;
                QFileInfo outInfo(outFile);
                auto txtFile = new QTemporaryFile(outInfo.dir().filePath("XXXXXX.txt"));

                if (!txtFile->open()) {
                    LOG_ERROR() << "Failed to create temp text file" << txtFile->fileName();
                    txtFile->deleteLater();
                    return;
                }
                txtFile->write(toPlainText().toUtf8());
                txtFile->close();

                const auto lang = m_speechDialog->languageCode();
                const auto voice = m_speechDialog->voiceCode();
                const auto spd = m_speechDialog->speed();
                auto job = new KokorodokiJob(txtFile->fileName(), outFile, lang, voice, spd);

                txtFile->setParent(job); // auto-delete with job
                job->setPostJobAction(new OpenPostJobAction(outFile, outFile, QString()));
                JOBS.add(job);
            });
        });
#endif

        connect(this, &QWidget::customContextMenuRequested, this, [=](const QPoint &pos) {
            std::unique_ptr<QMenu> menu{createStandardContextMenu()};
            actions().at(0)->setEnabled(Settings.notesZoom() > 0);
            menu->addActions(actions());
            menu->exec(mapToGlobal(pos));
        });
    }

    void setZoom(int delta)
    {
        auto zoom = Settings.notesZoom();
        zoomIn((zoom + delta >= 0) ? delta : -zoom);
        Settings.setNotesZoom(std::max<int>(0, zoom + delta));
    }

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        if (event->modifiers() & Qt::ControlModifier) {
            setZoom((event->angleDelta().y() < 0) ? -1 : 1);
            event->accept();
        } else {
            QPlainTextEdit::wheelEvent(event);
        }
    }

private:
    std::unique_ptr<SpeechDialog> m_speechDialog;
};

NotesDock::NotesDock(QWidget *parent)
    : QDockWidget(tr("Notes"), parent)
    , m_textEdit(new TextEditor(this))
    , m_blockUpdate(false)
{
    LOG_DEBUG() << "begin";
    setObjectName("NotesDock");
    QIcon filterIcon = QIcon::fromTheme("document-edit",
                                        QIcon(":/icons/oxygen/32x32/actions/document-edit.png"));
    setWindowIcon(filterIcon);
    toggleViewAction()->setIcon(windowIcon());
    setWhatsThis("https://forum.shotcut.org/t/notes-panel/33110/1");

    QObject::connect(m_textEdit, SIGNAL(textChanged()), SLOT(onTextChanged()));
    // Wrap the text editor with a container so we can place a toolbar beneath it.
    auto container = new QWidget(this);
    auto layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_textEdit, /*stretch*/ 1);

    // Create a DockToolBar to hold custom actions defined in TextEditor.
    auto toolbar = new DockToolBar(tr("Notes Controls"), container);
    toolbar->setAreaHint(Qt::BottomToolBarArea);
    const auto actions = m_textEdit->actions();
    toolbar->addAction(actions.at(0)); // Decrease Text Size
    toolbar->addAction(actions.at(1)); // Increase Text Size
#ifdef EXTERNAL_LAUNCHERS
    toolbar->addSeparator();
    toolbar->addAction(actions.at(2)); // Text to Speech
#endif
    layout->addWidget(toolbar, /*stretch*/ 0);
    QDockWidget::setWidget(container);

    LOG_DEBUG() << "end";
}

QString NotesDock::getText()
{
    return m_textEdit->toPlainText();
}

void NotesDock::setText(const QString &text)
{
    m_blockUpdate = true;
    m_textEdit->setPlainText(text);
    m_blockUpdate = false;
}

void NotesDock::onTextChanged()
{
    if (!m_blockUpdate) {
        emit modified();
    }
}
