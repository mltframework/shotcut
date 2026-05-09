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

#ifndef ADDONSERVICEMODEL_H
#define ADDONSERVICEMODEL_H

#include <QAbstractListModel>
#include <QStringList>

class AddOnServiceModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ModelRoles {
        ServiceRole = Qt::UserRole + 1,
        TitleRole,
        DescriptionRole,
        IsAudioRole,
        SupportsRgbaRole,
        SupportsYuvRole,
        SupportsTenBitRole,
        EnabledRole,
    };

    explicit AddOnServiceModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool isEnabled(const QString &service) const;
    Q_INVOKABLE void setEnabled(const QString &service, bool enabled);
    Q_INVOKABLE void setEnabledServices(const QStringList &services);
    Q_INVOKABLE QStringList enabledServices() const;

signals:
    void enabledServicesChanged();

private:
    struct Item
    {
        QString service;
        QString title;
        QString description;
        bool isAudio{false};
        bool supportsRgba{false};
        bool supportsYuv{false};
        bool supportsTenBit{false};
    };

    QList<Item> m_items;
    QStringList m_enabledServices;

    int indexOfService(const QString &service) const;
    void saveEnabledServices();
};

#endif // ADDONSERVICEMODEL_H
