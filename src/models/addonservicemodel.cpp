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

#include "addonservicemodel.h"

#include "Logger.h"
#include "mltcontroller.h"
#include "settings.h"

#include <MltProperties.h>
#include <QScopedPointer>

#include <algorithm>

AddOnServiceModel::AddOnServiceModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_enabledServices = Settings.addOnFilterServices();
}

int AddOnServiceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant AddOnServiceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const auto &item = m_items.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
        return item.title;
    case ServiceRole:
        return item.service;
    case DescriptionRole:
        return item.description;
    case IsAudioRole:
        return item.isAudio;
    case SupportsRgbaRole:
        return item.supportsRgba;
    case SupportsYuvRole:
        return item.supportsYuv;
    case SupportsTenBitRole:
        return item.supportsTenBit;
    case EnabledRole:
        return m_enabledServices.contains(item.service);
    default:
        break;
    }

    return QVariant();
}

QHash<int, QByteArray> AddOnServiceModel::roleNames() const
{
    auto roles = QAbstractListModel::roleNames();
    roles[ServiceRole] = "service";
    roles[TitleRole] = "title";
    roles[DescriptionRole] = "description";
    roles[IsAudioRole] = "isAudio";
    roles[SupportsRgbaRole] = "supportsRgba";
    roles[SupportsYuvRole] = "supportsYuv";
    roles[SupportsTenBitRole] = "supportsTenBit";
    roles[EnabledRole] = "enabled";
    return roles;
}

void AddOnServiceModel::reload()
{
    beginResetModel();
    m_items.clear();

    // Refresh persisted selections when opening the management dialog.
    m_enabledServices = Settings.addOnFilterServices();

    QScopedPointer<Mlt::Properties> mltFilters(MLT.repository()->filters());
    if (!mltFilters || !mltFilters->is_valid()) {
        LOG_WARNING() << "Failed to query MLT filter services";
        endResetModel();
        return;
    }

    for (int i = 0; i < mltFilters->count(); ++i) {
        const char *name = mltFilters->get_name(i);
        if (!name || !*name)
            continue;

        QString service = QString::fromLatin1(name);
        QScopedPointer<Mlt::Properties> metadata(
            MLT.repository()->metadata(mlt_service_filter_type, name));
        if (!metadata || !metadata->is_valid())
            continue;

        Item item;
        item.service = service;
        item.title = QString::fromUtf8(metadata->get("title"));
        if (item.title.isEmpty())
            item.title = service;

        item.description = QString::fromUtf8(metadata->get("description"));

        Mlt::Properties tags(metadata->get_data("tags"));
        if (tags.is_valid()) {
            for (int t = 0; t < tags.count(); ++t) {
                if (!qstricmp(tags.get(t), "Audio")) {
                    item.isAudio = true;
                    break;
                }
            }
        }

        Mlt::Properties imageFormats(metadata->get_data("image_formats"));
        if (imageFormats.is_valid()) {
            for (int f = 0; f < imageFormats.count(); ++f) {
                const QString format = QString::fromUtf8(imageFormats.get(f));
                if (format == QStringLiteral("rgba"))
                    item.supportsRgba = true;
                else if (format == QStringLiteral("rgba64")) {
                    item.supportsRgba = true;
                    item.supportsTenBit = true;
                } else if (format == QStringLiteral("yuv422")
                           || format == QStringLiteral("yuv420p")) {
                    item.supportsYuv = true;
                } else if (format == QStringLiteral("yuv422p16")
                           || format == QStringLiteral("yuv420p10")
                           || format == QStringLiteral("yuv444p10")) {
                    item.supportsYuv = true;
                    item.supportsTenBit = true;
                }
            }
        }

        m_items.push_back(item);
    }

    std::sort(m_items.begin(), m_items.end(), [](const Item &a, const Item &b) {
        return a.title.toLower() < b.title.toLower();
    });

    m_enabledServices.erase(std::remove_if(m_enabledServices.begin(),
                                           m_enabledServices.end(),
                                           [this](const QString &service) {
                                               return indexOfService(service) < 0;
                                           }),
                            m_enabledServices.end());

    endResetModel();
    saveEnabledServices();
}

bool AddOnServiceModel::isEnabled(const QString &service) const
{
    return m_enabledServices.contains(service);
}

void AddOnServiceModel::setEnabled(const QString &service, bool enabled)
{
    int row = indexOfService(service);
    if (row < 0)
        return;

    bool wasEnabled = m_enabledServices.contains(service);
    if (wasEnabled == enabled)
        return;

    if (enabled)
        m_enabledServices << service;
    else
        m_enabledServices.removeAll(service);

    saveEnabledServices();
    emit enabledServicesChanged();

    QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, QVector<int>() << EnabledRole);
}

void AddOnServiceModel::setEnabledServices(const QStringList &services)
{
    QStringList normalized = services;
    normalized.removeDuplicates();

    QStringList oldEnabled = m_enabledServices;
    m_enabledServices.clear();
    for (const auto &service : normalized) {
        if (indexOfService(service) >= 0)
            m_enabledServices << service;
    }

    bool changed = (oldEnabled != m_enabledServices);
    saveEnabledServices();

    for (int row = 0; row < m_items.size(); ++row) {
        const QString &service = m_items.at(row).service;
        bool wasEnabled = oldEnabled.contains(service);
        bool isEnabledNow = m_enabledServices.contains(service);
        if (wasEnabled != isEnabledNow) {
            QModelIndex modelIndex = index(row, 0);
            emit dataChanged(modelIndex, modelIndex, QVector<int>() << EnabledRole);
        }
    }

    if (changed)
        emit enabledServicesChanged();
}

QStringList AddOnServiceModel::enabledServices() const
{
    return m_enabledServices;
}

int AddOnServiceModel::indexOfService(const QString &service) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).service == service)
            return i;
    }
    return -1;
}

void AddOnServiceModel::saveEnabledServices()
{
    QStringList services = m_enabledServices;
    services.removeDuplicates();
    Settings.setAddOnFilterServices(services);
}
