/*
 * Copyright (c) 2021-2022 Meltytech, LLC
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

#include "multifileexportdialog.h"

#include "mainwindow.h"
#include "proxymanager.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include "qmltypes/qmlapplication.h"

#include <MltPlaylist.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

enum {
    NAME_FIELD_NONE = 0,
    NAME_FIELD_NAME,
    NAME_FIELD_INDEX,
    NAME_FIELD_DATE,
    NAME_FIELD_HASH,
};

MultiFileExportDialog::MultiFileExportDialog(QString title, Mlt::Playlist *playlist,
                                             const QString &directory, const QString &prefix, const QString &extension, QWidget *parent)
    : QDialog(parent)
    , m_playlist(playlist)
{
    int col = 0;
    setWindowTitle(title);
    setWindowModality(QmlApplication::dialogModality());

    QGridLayout *glayout = new QGridLayout();
    glayout->setHorizontalSpacing(4);
    glayout->setVerticalSpacing(2);
    // Directory
    glayout->addWidget(new QLabel(tr("Directory")), col, 0, Qt::AlignRight);
    QHBoxLayout *dirHbox = new QHBoxLayout();
    m_dir = new QLineEdit(QDir::toNativeSeparators(directory));
    m_dir->setReadOnly(true);
    QPushButton *browseButton = new QPushButton(this);
    browseButton->setIcon(QIcon::fromTheme("document-open",
                                           QIcon(":/icons/oxygen/32x32/actions/document-open.png")));
    if (!connect(browseButton, &QAbstractButton::clicked, this, &MultiFileExportDialog::browse))
        connect(browseButton, SIGNAL(clicked()), SLOT(browse()));
    dirHbox->addWidget(m_dir);
    dirHbox->addWidget(browseButton);
    glayout->addLayout(dirHbox, col++, 1, Qt::AlignLeft);
    // Prefix
    glayout->addWidget(new QLabel(tr("Prefix")), col, 0, Qt::AlignRight);
    m_prefix = new QLineEdit(prefix.isEmpty() ? tr("export") : prefix);
    if (!connect(m_prefix, &QLineEdit::textChanged, this, &MultiFileExportDialog::rebuildList))
        connect(m_prefix, SIGNAL(textChanged(const QString &)), SLOT(rebuildList()));
    glayout->addWidget(m_prefix, col++, 1, Qt::AlignLeft);
    // Field 1
    glayout->addWidget(new QLabel(tr("Field 1")), col, 0, Qt::AlignRight);
    m_field1 = new QComboBox();
    fillCombo(m_field1);
    if (!connect(m_field1, QOverload<int>::of(&QComboBox::activated), this,
                 &MultiFileExportDialog::rebuildList))
        connect(m_field1, SIGNAL(activated(const QString &)), SLOT(rebuildList()));
    glayout->addWidget(m_field1, col++, 1, Qt::AlignLeft);
    // Field 2
    glayout->addWidget(new QLabel(tr("Field 2")), col, 0, Qt::AlignRight);
    m_field2 = new QComboBox();
    fillCombo(m_field2);
    if (!connect(m_field2, QOverload<int>::of(&QComboBox::activated), this,
                 &MultiFileExportDialog::rebuildList))
        connect(m_field2, SIGNAL(activated(const QString &)), SLOT(rebuildList()));
    glayout->addWidget(m_field2, col++, 1, Qt::AlignLeft);
    // Field 3
    glayout->addWidget(new QLabel(tr("Field 3")), col, 0, Qt::AlignRight);
    m_field3 = new QComboBox();
    fillCombo(m_field3);
    m_field3->setCurrentIndex(NAME_FIELD_INDEX);
    if (!connect(m_field3, QOverload<int>::of(&QComboBox::activated), this,
                 &MultiFileExportDialog::rebuildList))
        connect(m_field3, SIGNAL(activated(const QString &)), SLOT(rebuildList()));
    glayout->addWidget(m_field3, col++, 1, Qt::AlignLeft);
    // Extension
    glayout->addWidget(new QLabel(tr("Extension")), col, 0, Qt::AlignRight);
    m_ext = new QLineEdit(extension);
    if (!connect(m_ext, &QLineEdit::textChanged, this, &MultiFileExportDialog::rebuildList))
        connect(m_ext, SIGNAL(textChanged(const QString &)), SLOT(rebuildList()));
    glayout->addWidget(m_ext, col++, 1, Qt::AlignLeft);
    // Error
    m_errorIcon = new QLabel();
    QIcon icon = QIcon(":/icons/oxygen/32x32/status/task-reject.png");
    m_errorIcon->setPixmap(icon.pixmap(QSize(24, 24)));
    glayout->addWidget(m_errorIcon, col, 0, Qt::AlignRight);
    m_errorText = new QLabel();
    glayout->addWidget(m_errorText, col++, 1, Qt::AlignLeft);
    // List
    m_list = new QListWidget();
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setIconSize(QSize(16, 16));
    glayout->addWidget(m_list, col++, 0, 1, 2);
    // Buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    glayout->addWidget(m_buttonBox, col++, 0, 1, 2);
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    glayout->setColumnMinimumWidth(1,
                                   fontMetrics().horizontalAdvance(m_dir->text()) + browseButton->width());

    this->setLayout (glayout);
    this->setModal(true);

    rebuildList();

    resize(400, 300);
}

QStringList MultiFileExportDialog::getExportFiles()
{
    return m_stringList;
}

QString MultiFileExportDialog::appendField(QString text, QComboBox *combo, int clipIndex)
{
    QString field;
    switch (combo->currentData().toInt()) {
    default:
    case NAME_FIELD_NONE:
        break;
    case NAME_FIELD_NAME: {
        QScopedPointer<Mlt::ClipInfo> info(MAIN.playlist()->clip_info(clipIndex));
        if (info && info->producer && info->producer->is_valid()) {
            field = info->producer->get(kShotcutCaptionProperty);
            if (field.isEmpty()) {
                field = ProxyManager::resource(*info->producer);
                field = QFileInfo(field).completeBaseName();
            }
            if (field == "<producer>") {
                field = QString::fromUtf8(info->producer->get("mlt_service"));
            }
        }
        break;
    }
    case NAME_FIELD_INDEX: {
        int digits = QString::number(m_playlist->count()).size();
        field = QString("%1").arg(clipIndex + 1, digits, 10, QChar('0'));
        break;
    }
    case NAME_FIELD_DATE: {
        QScopedPointer<Mlt::ClipInfo> info(MAIN.playlist()->clip_info(clipIndex));
        if (info && info->producer && info->producer->is_valid()) {
            int64_t ms = info->producer->get_creation_time();
            if (ms) {
                field = QDateTime::fromMSecsSinceEpoch(ms).toString("yyyyMMdd-HHmmss");
            }
        }
        break;
    }
    case NAME_FIELD_HASH: {
        QScopedPointer<Mlt::ClipInfo> info(MAIN.playlist()->clip_info(clipIndex));
        field = Util::getHash(*info->producer);
        break;
    }
    }

    if (text.isEmpty()) {
        return field;
    } else if (field.isEmpty()) {
        return text;
    } else {
        return text + "-" + field;
    }
}

void MultiFileExportDialog::fillCombo(QComboBox *combo)
{
    combo->addItem(tr("None"), QVariant(NAME_FIELD_NONE));
    combo->addItem(tr("Name"), QVariant(NAME_FIELD_NAME));
    combo->addItem(tr("Index"), QVariant(NAME_FIELD_INDEX));
    combo->addItem(tr("Date"), QVariant(NAME_FIELD_DATE));
    combo->addItem(tr("Hash"), QVariant(NAME_FIELD_HASH));
}

void MultiFileExportDialog::rebuildList()
{
    m_stringList.clear();
    m_list->clear();
    for (int i = 0; i < m_playlist->count(); i++) {
        QString filename = m_prefix->text();
        filename = appendField(filename, m_field1, i);
        filename = appendField(filename, m_field2, i);
        filename = appendField(filename, m_field3, i);
        if (!filename.isEmpty()) {
            filename = m_dir->text() + QDir::separator() + filename + "." + m_ext->text();
            m_stringList << filename;
        }
    }
    m_list->addItems(m_stringList);

    // Detect Errors
    m_errorText->setText("");
    int n = m_stringList.size();
    if (n == 0) {
        m_errorText->setText(tr("Empty File Name"));
    } else if (!QDir(m_dir->text()).exists()) {
        m_errorText->setText(tr("Directory does not exist: %1").arg(m_dir->text()));
    } else {
        // Search for existing or duplicate files
        for (int i = 0; i < n; i++) {
            QString errorString;
            QFileInfo fileInfo(m_stringList[i]);
            if (fileInfo.exists()) {
                errorString = tr("File Exists: %1").arg(m_stringList[i]);
            } else {
                for (int j = 0; j < n; j++) {
                    if (j != i && m_stringList[i] == m_stringList[j]) {
                        QString filename = QFileInfo(m_stringList[i]).fileName();
                        errorString = tr("Duplicate File Name: %1").arg(filename);
                        break;
                    }
                }
            }

            QListWidgetItem *item = m_list->item(i);
            if (errorString.isEmpty()) {
                item->setIcon(QIcon(":/icons/oxygen/32x32/status/task-complete.png"));
            } else {
                item->setIcon(QIcon(":/icons/oxygen/32x32/status/task-reject.png"));
                item->setToolTip(errorString);
                m_errorText->setText(errorString);
            }
        }
    }

    if (m_errorText->text().isEmpty()) {
        m_errorText->setVisible(false);
        m_errorIcon->setVisible(false);
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        m_errorText->setVisible(true);
        m_errorIcon->setVisible(true);
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    m_buttonBox->button(QDialogButtonBox::Ok)->setToolTip(tr("Fix file name errors before export."));
}

void MultiFileExportDialog::browse()
{
    QString directory = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this,
                                                                                   tr("Export Directory"), m_dir->text(), Util::getFileDialogOptions()));
    if (!directory.isEmpty()) {
        m_dir->setText(directory);
        rebuildList();
    }
}
