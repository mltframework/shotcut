/*
 * Copyright (c) 2013-2023 Meltytech, LLC
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

#ifndef ATTACHEDFILTERSMODEL_H
#define ATTACHEDFILTERSMODEL_H

#include <QAbstractListModel>
#include <MltFilter.h>
#include <MltProducer.h>
#include <MltEvent.h>

class QmlMetadata;

class AttachedFiltersModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString producerTitle READ producerTitle NOTIFY trackTitleChanged)
    Q_PROPERTY(bool isProducerSelected READ isProducerSelected NOTIFY isProducerSelectedChanged)
    Q_PROPERTY(bool supportsLinks READ supportsLinks NOTIFY supportsLinksChanged)
public:
    enum ModelRoles {
        TypeDisplayRole = Qt::UserRole + 1,
        PluginTypeRole,
    };

    explicit AttachedFiltersModel(QObject *parent = 0);

    Mlt::Service *getService(int row) const;
    QmlMetadata *getMetadata(int row) const;
    void setProducer(Mlt::Producer *producer = 0);
    QString producerTitle() const;
    bool isProducerSelected() const;
    bool isSourceClip() const;
    bool supportsLinks() const;
    Mlt::Producer *producer() const
    {
        return m_producer.data();
    }
    QString name(int row) const;

    // The below are used by QUndoCommands
    void doAddService(Mlt::Producer &producer, Mlt::Service &service, int row);
    void doRemoveService(Mlt::Producer &producer, int row);
    void doMoveService(Mlt::Producer &producer, int fromRow, int toRow);
    void doSetDisabled(Mlt::Producer &producer, int row, bool disable);
    Mlt::Service doGetService(Mlt::Producer &producer, int row);

    // QAbstractListModel Implementation
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    QHash<int, QByteArray> roleNames() const;
    Qt::DropActions supportedDropActions() const;
    bool insertRows(int row, int count, const QModelIndex &parent);
    bool removeRows(int row, int count, const QModelIndex &parent);
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                  const QModelIndex &destinationParent, int destinationRow);

signals:
    void changed();
    void duplicateAddFailed(int index);
    void trackTitleChanged();
    void isProducerSelectedChanged();
    void supportsLinksChanged();
    void addedOrRemoved(Mlt::Producer *);
    void requestConvert(QString, bool set709Convert, bool withSubClip);

public slots:
    int add(QmlMetadata *meta);
    int addService(Mlt::Service *service);
    void remove(int row);
    bool move(int fromRow, int toRow);

private:
    static void producerChanged(mlt_properties owner, AttachedFiltersModel *model);
    void reset(Mlt::Producer *producer = 0);
    bool isProducerLoaded(Mlt::Producer &producer) const;
    int findInsertRow(QmlMetadata *meta);
    Mlt::Producer getFilterSetProducer(QmlMetadata *meta);

    int m_dropRow;
    int m_removeRow;
    QScopedPointer<Mlt::Producer> m_producer;
    QScopedPointer<Mlt::Event> m_event;
    typedef QList<QmlMetadata *> MetadataList;
    MetadataList m_metaList;
};

#endif // ATTACHEDFILTERSMODEL_H
