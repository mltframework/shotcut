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

#ifndef COPRODUCERSETTINGSDIALOG_H
#define COPRODUCERSETTINGSDIALOG_H

#include <QDialog>

#include "models/extensionmodel.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QToolButton;
class QTreeView;
class QWidget;

class CoproducerSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CoproducerSettingsDialog(QWidget *parent = nullptr);

private slots:
    void onProviderChanged(int index);
    void onModelRowClicked(const QModelIndex &index);
    void onModelContextMenu(const QPoint &pos);
    void onBrowseModelClicked();
    void onRefreshModelsClicked();
    void onRefreshOpenAiModelsClicked();
    void onAccepted();

private:
    void downloadModel(int index);
    void setCurrentModel(int index);
    void updateModelStatus();
    void refreshOpenAiModels(bool silent);
    void maybeAutoRefreshOpenAiModels();
    void setLocalModelPath(const QString &path);
    QString currentModelId() const;
    QString currentBaseUrl() const;

    QComboBox *m_providerCombo;
    QLabel *m_modelLabel = nullptr;
    QLabel *m_apiKeyLabel = nullptr;
    QLabel *m_baseUrlLabel = nullptr;
    QComboBox *m_modelEdit;
    QComboBox *m_baseUrlEdit;
    QLineEdit *m_apiKeyEdit;
    QLineEdit *m_localModelLabel;
    QWidget *m_localConfigWidget;
    QTreeView *m_modelTable;
    QToolButton *m_openAiRefreshButton = nullptr;
    ExtensionModel m_model;
    QNetworkAccessManager *m_network;
    QString m_lastAutoRefreshKey;
};

#endif // COPRODUCERSETTINGSDIALOG_H
