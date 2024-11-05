/*
 * Copyright (c) 2021-2023 Meltytech, LLC
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

#include "saveimagedialog.h"

#include "mltcontroller.h"
#include "settings.h"
#include "util.h"
#include <Logger.h>

#include <QDebug>
#include <QtMath>

static QString suffixFromFilter(const QString &filterText)
{
    QString suffix = filterText.section("*", 1, 1).section(")", 0, 0).section(" ", 0, 0);
    if (!suffix.startsWith(".") ) {
        suffix.clear();
    }
    return suffix;
}

SaveImageDialog::SaveImageDialog(QWidget *parent, const QString &caption, QImage &image)
    : QFileDialog(parent, caption)
    , m_image(image)
{
    setModal(true);
    setAcceptMode(QFileDialog::AcceptSave);
    setFileMode(QFileDialog::AnyFile);
    setOptions(Util::getFileDialogOptions());
    setDirectory(Settings.savePath());

    QString nameFilter =
        tr("PNG (*.png);;BMP (*.bmp);;JPEG (*.jpg *.jpeg);;PPM (*.ppm);;TIFF (*.tif *.tiff);;WebP (*.webp);;All Files (*)");
    setNameFilter(nameFilter);

    QStringList nameFilters = nameFilter.split(";;");
    QString suffix = Settings.exportFrameSuffix();
    QString selectedNameFilter = nameFilters[0];
    for (const auto &f : nameFilters) {
        if (f.contains(suffix.toLower())) {
            selectedNameFilter = f;
            break;
        }
    }
    selectNameFilter(selectedNameFilter);

    // Use the current player time as a suggested file name
    QString nameSuggestion = QStringLiteral("Shotcut_%1").arg(MLT.producer()->frame_time(
                                                                  mlt_time_clock));
    nameSuggestion = nameSuggestion.replace(":", "_");
    nameSuggestion = nameSuggestion.replace(".", "_");
    nameSuggestion += suffix;
    selectFile(nameSuggestion);

#if !defined(Q_OS_WIN)
    if (!connect(this, &QFileDialog::filterSelected, this, &SaveImageDialog::onFilterSelected))
        connect(this, SIGNAL(filterSelected(const QString &)),
                SLOT(const onFilterSelected(const QString &)));
#endif
    if (!connect(this, &QFileDialog::fileSelected, this, &SaveImageDialog::onFileSelected))
        connect(this, SIGNAL(fileSelected(const QString &)), SLOT(onFileSelected(const QString &)));
}

void SaveImageDialog::onFilterSelected(const QString &filter)
{
    // When the file type filter is changed, automatically change
    // the file extension to match.
    if (filter.isEmpty()) {
        return;
    }
    QString suffix = suffixFromFilter(filter);
    if (suffix.isEmpty()) {
        return; // All files
    }
    QStringList files = selectedFiles();
    if (files.size() == 0) {
        return;
    }
    QString filename = files[0];
    // Strip the suffix from the current file name
    if (!QFileInfo(filename).suffix().isEmpty()) {
        filename = filename.section(".", 0, -2);
    }
    // Add the new suffix
    filename += suffix;
    selectFile(filename);
}

void SaveImageDialog::onFileSelected(const QString &file)
{
    if (file.isEmpty()) {
        return;
    }
    m_saveFile = file;
    QFileInfo fi(m_saveFile);
    if (fi.suffix().isEmpty()) {
        QString suffix = suffixFromFilter(selectedNameFilter());
        if ( suffix.isEmpty() ) {
            suffix = ".png";
        }
        m_saveFile += suffix;
        fi = QFileInfo(m_saveFile);
    }
    if (Util::warnIfNotWritable(m_saveFile, this, windowTitle()))
        return;
    // Convert to square pixels if needed.
    qreal aspectRatio = (qreal) m_image.width() / m_image.height();
    if (qFloor(aspectRatio * 1000) != qFloor(MLT.profile().dar() * 1000)) {
        m_image = m_image.scaled(qRound(m_image.height() * MLT.profile().dar()), m_image.height(),
                                 Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    m_image.save(m_saveFile, Q_NULLPTR, (fi.suffix() == "webp") ? 80 : -1);
    Settings.setSavePath(fi.path());
    Settings.setExportFrameSuffix(QStringLiteral(".") + fi.suffix());
}
