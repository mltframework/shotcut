/*
 * Copyright (c) 2014-2023 Meltytech, LLC
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

#ifndef METADATAMODEL_H
#define METADATAMODEL_H

#include <QAbstractListModel>
#include <QList>

class QmlMetadata;

class MetadataModel : public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(MetadataFilter)
    Q_PROPERTY(MetadataFilter filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)

public:

    enum ModelRoles {
        NameRole = Qt::UserRole + 1,
        HiddenRole,
        FavoriteRole,
        ServiceRole,
        IsAudioRole,
        NeedsGpuRole,
        VisibleRole,
        PluginTypeRole,
    };

    enum MetadataFilter {
        NoFilter,
        FavoritesFilter,
        VideoFilter,
        AudioFilter,
        LinkFilter,
        FilterSetFilter,
        GPUFilter,
    };

    enum FilterMaskBits {
        HiddenMaskBit = 1 << 0,
        clipOnlyMaskBit = 1 << 1,
        gpuIncompatibleMaskBit = 1 << 2,
        gpuAlternativeMaskBit = 1 << 3,
        needsGPUMaskBit = 1 << 4,
        linkMaskBit = 1 << 5,
        trackOnlyMaskBit = 1 << 6,
        outputOnlyMaskBit = 1 << 7,
    };

    explicit MetadataModel(QObject *parent = 0);

    // Implement QAbstractListModel
    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QHash<int, QByteArray> roleNames() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    // Direct access to QmlMetadata
    void add(QmlMetadata *data);
    Q_INVOKABLE QmlMetadata *get(int index) const;
    MetadataFilter filter() const
    {
        return m_filter;
    }
    void setFilter(MetadataFilter);
    QString search() const
    {
        return m_search;
    }
    void setSearch(const QString &search);
    Q_INVOKABLE bool isVisible(int row) const;
    void updateFilterMask(bool isClipProducer, bool isChainProducer, bool isTrackProducer,
                          bool isOutputProducer);
    Q_INVOKABLE void saveFilterSet(const QString &name);
    Q_INVOKABLE void deleteFilterSet(const QString &name);

signals:
    void filterChanged();
    void searchChanged();

private:
    typedef QList<QmlMetadata *> MetadataList;
    MetadataList m_list;
    MetadataFilter m_filter;
    bool m_isClipProducer;
    bool m_isChainProducer;
    bool m_isTrackProducer;
    bool m_isOutputProducer;
    QString m_search;
    unsigned m_filterMask;

    unsigned computeFilterMask(const QmlMetadata *meta);
};

#endif // METADATAMODEL_H
