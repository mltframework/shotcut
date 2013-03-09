/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * GL shader based on BSD licensed code from Peter Bengtsson:
 * http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
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

#include "attachedfiltersmodel.h"
#include "mltcontroller.h"

AttachedFiltersModel::AttachedFiltersModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_rows(0)
{
}

void AttachedFiltersModel::calculateRows()
{
    Mlt::Producer* producer = MLT.producer();
    m_rows =-0;
    if (MLT.isPlaylist()) return;
    if (producer && producer->is_valid()) {
        int n = producer->filter_count();
        while (n--) {
            Mlt::Filter* filter = producer->filter(n);
            if (filter && filter->is_valid() && !filter->get_int("_loader"))
                m_rows++;
            delete filter;
        }
    }
}

Mlt::Filter* AttachedFiltersModel::filterForRow(int row) const
{
    Mlt::Filter* result = 0;
    Mlt::Producer* producer = MLT.producer();
    if (producer && producer->is_valid()) {
        int count = producer->filter_count();
        int j = 0;
        for (int i = 0; i < count; i++) {
            Mlt::Filter* filter = producer->filter(i);
            if (filter && filter->is_valid() && !filter->get_int("_loader")) {
                if (j == row) {
                    result = filter;
                    break;
                }
                j++;
            }
            delete filter;
        }
    }
    return result;
}

int AttachedFiltersModel::rowCount(const QModelIndex &parent) const
{
    if (MLT.producer() && MLT.producer()->is_valid())
        return m_rows;
    else
        return 0;
}

Qt::ItemFlags AttachedFiltersModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
}

QVariant AttachedFiltersModel::data(const QModelIndex &index, int role) const
{
    if ( !MLT.producer() || !MLT.producer()->is_valid()
        || index.row() >= MLT.producer()->filter_count())
        return QVariant();
    switch (role ) {
    case Qt::DisplayRole: {
            Mlt::Filter* filter = filterForRow(index.row());
            QVariant result;
            if (filter && filter->is_valid() && filter->get("mlt_service"))
                result = QString::fromUtf8(filter->get("mlt_service"));
            delete filter;
            if (result == "movit.blur" || result == "boxblur")
                result = tr("Blur");
            else if (result == "movit.diffusion")
                result = tr("Diffusion");
            else if (result == "movit.glow" || result == "frei0r.glow")
                result = tr("Glow");
            else if (result == "movit.mirror" || result == "mirror")
                result = tr("Mirror");
            else if (result == "movit.sharpen" || result == "frei0r.sharpness")
                result = tr("Sharpen");
            else if (result == "movit.vignette" || result == "vignette")
                result = tr("Vignette");
            return result;
        }
    case Qt::CheckStateRole: {
            Mlt::Filter* filter = filterForRow(index.row());
            QVariant result = Qt::Unchecked;
            if (filter && filter->is_valid() && !filter->get_int("disable"))
                result = Qt::Checked;
            delete filter;
            return result;
        }
        break;
    default:
        break;
    }
    return QVariant();
}

bool AttachedFiltersModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::CheckStateRole) {
        Mlt::Filter* filter = filterForRow(index.row());
        if (filter && filter->is_valid()) {
            double speed = MLT.producer()->get_speed();
            MLT.consumer()->stop();
            filter->set("disable", !filter->get_int("disable"));
            MLT.play(speed);
            emit changed();
            emit dataChanged(createIndex(index.row(), 0), createIndex(index.row(), 0));
        }
        delete filter;
        return true;
    }
    return false;
}

Mlt::Filter *AttachedFiltersModel::add(const QString& name)
{
    Mlt::Filter* filter = new Mlt::Filter(MLT.profile(), name.toUtf8().constData());
    if (filter->is_valid()) {
        int count = rowCount();
        double speed = MLT.producer()->get_speed();
        MLT.consumer()->stop();

        if (name == "frei0r.glow") {
            filter->set("Blur", 0.5);
        }
        else if (name == "frei0r.sharpness") {
            filter->set("Amount", 0.5);
            filter->set("Size", 0.5);
        }

        beginInsertRows(QModelIndex(), count, count);
        MLT.producer()->attach(*filter);
        m_rows++;
        endInsertRows();

        MLT.play(speed);
        emit changed();
    }
    return filter;
}

void AttachedFiltersModel::remove(int row)
{
    Mlt::Filter* filter = filterForRow(row);
    if (filter && filter->is_valid()) {
        double speed = MLT.producer()->get_speed();
        MLT.consumer()->stop();

        beginRemoveRows(QModelIndex(), row, row);
        MLT.producer()->detach(*filter);
        m_rows--;
        endRemoveRows();

        MLT.play(speed);
        emit changed();
    }
    delete filter;
}

void AttachedFiltersModel::reset()
{
    calculateRows();
    beginResetModel();
    endResetModel();
}
