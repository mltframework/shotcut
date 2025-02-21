/*
 * Copyright (c) 2014-2024 Meltytech, LLC
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

#include <QList>
#include <QSortFilterProxyModel>

class QmlMetadata;

class MetadataModel : public QSortFilterProxyModel
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
        reverseMaskBit = 1 << 8,
    };

    explicit MetadataModel(QObject *parent = 0);

    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int sourceRowCount(const QModelIndex &parent = QModelIndex()) const;
    void add(QmlMetadata *data);
    Q_INVOKABLE QmlMetadata *get(int row) const;
    QmlMetadata *getFromSource(int index) const;
    Q_INVOKABLE void saveFilterSet(const QString &name);
    Q_INVOKABLE void deleteFilterSet(const QString &name);
    MetadataFilter filter() const { return m_filter; }
    void setFilter(MetadataFilter);
    void updateFilterMask(bool isClipProducer,
                          bool isChainProducer,
                          bool isTrackProducer,
                          bool isOutputProducer,
                          bool isReverseSupported);
    QString search() const { return m_search; }
    void setSearch(const QString &search);

signals:
    void filterChanged();
    void searchChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    MetadataFilter m_filter;
    unsigned m_filterMask;
    QString m_search;
    bool m_isClipProducer;
    bool m_isChainProducer;
    bool m_isTrackProducer;
    bool m_isOutputProducer;
    bool m_isReverseSupported;
};

class InternalMetadataModel : public QAbstractListModel
{
public:
    explicit InternalMetadataModel(QObject *parent = 0)
        : QAbstractListModel(parent){};

    // Implement QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QHash<int, QByteArray> roleNames() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    // Direct access to QmlMetadata
    void add(QmlMetadata *data);
    QmlMetadata *get(int index) const;
    QList<QmlMetadata *> &list() { return m_list; }
    void remove(int index);

private:
    typedef QList<QmlMetadata *> MetadataList;
    MetadataList m_list;

    unsigned computeFilterMask(const QmlMetadata *meta);
};

#endif // METADATAMODEL_H
