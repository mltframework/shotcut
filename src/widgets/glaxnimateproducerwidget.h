/*
 * Copyright (c) 2022 Meltytech, LLC
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

#ifndef GLAXNIMATEPRODUCERWIDGET_H
#define GLAXNIMATEPRODUCERWIDGET_H

#include <QWidget>
#include "abstractproducerwidget.h"

namespace Ui {
class GlaxnimateProducerWidget;
}
class QFileSystemWatcher;

class GlaxnimateProducerWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    explicit GlaxnimateProducerWidget(QWidget *parent = 0);
    ~GlaxnimateProducerWidget();

    // AbstractProducerWidget overrides
    Mlt::Producer *newProducer(Mlt::Profile &);
    virtual void setProducer(Mlt::Producer *);
    Mlt::Properties getPreset() const;
    void loadPreset(Mlt::Properties &);

signals:
    void producerChanged(Mlt::Producer *);
    void modified();

public slots:
    void rename();

private slots:
    void on_colorButton_clicked();
    void on_preset_selected(void *p);
    void on_preset_saveClicked();
    void on_lineEdit_editingFinished();
    void on_notesTextEdit_textChanged();
    void on_editButton_clicked();
    void onFileChanged(const QString &path);
    void on_reloadButton_clicked();
    void on_durationSpinBox_editingFinished();

private:
    void launchGlaxnimate(const QString &filename);

    Ui::GlaxnimateProducerWidget *ui;
    QString m_title;
    std::unique_ptr<QFileSystemWatcher> m_watcher;
};

#endif // GLAXNIMATEPRODUCERWIDGET_H
