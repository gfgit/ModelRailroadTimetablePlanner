/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
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
 *
 */

#ifndef SINGLEDEPTHREEMODELHELPER_H
#define SINGLEDEPTHREEMODELHELPER_H

#include <QAbstractItemModel>

/*!
 * \brief The SingleDepthTreeModelHelper class
 *
 * Helper class to implement single depth tree model like:
 *
 * Root
 * - TopLevel 1
 *   - Child 1
 *   - Child 2
 * - TopLevel 2
 *   - Child 1
 *
 * \tparam Super is the model itself, must have constant NCols
 * \tparam ModelData must have getTopLevelAtRow(), topLevelCount(), getParentRow(), getParent()
 * \tparam RowData must have childCount() and ptrForRow()
 */
template <typename Super, typename ModelData, typename RowData>
class SingleDepthTreeModelHelper : public QAbstractItemModel
{
public:
    SingleDepthTreeModelHelper(QObject *p = nullptr) : QAbstractItemModel(p) {}

    // Basic functionality:
    QModelIndex index(int row, int column,
                      const QModelIndex &p = QModelIndex()) const override
    {
        if(p.isValid())
        {
            if(p.row() >= m_data.topLevelCount() || p.internalPointer())
                return QModelIndex(); //Out of bound or child-most

            auto topLevel = m_data.getTopLevelAtRow(p.row());
            if(row >= topLevel->childCount())
                return QModelIndex();

            void *ptr = const_cast<void *>(static_cast<const void *>(topLevel->ptrForRow(row)));
            return createIndex(row, column, ptr);
        }

        if(row >= m_data.topLevelCount())
            return QModelIndex();

        return createIndex(row, column, nullptr);
    }

    QModelIndex parent(const QModelIndex &idx) const override
    {
        if(!idx.isValid())
            return QModelIndex();

        RowData *item = static_cast<RowData*>(idx.internalPointer());
        if(!item) //Caption, it's toplevel so no parent
            return QModelIndex();

        int topLevelRow = m_data.getParentRow(item);
        if(topLevelRow < 0)
            return QModelIndex();
        return index(topLevelRow, 0);
    }

    int rowCount(const QModelIndex &p) const override
    {
        if(p.isValid())
        {
            if(p.internalPointer())
                return 0; //Child most, no childs below so 0 rows

            auto topLevel = m_data.getTopLevelAtRow(p.row());
            return topLevel->childCount();
        }

        return m_data.topLevelCount();
    }

    int columnCount(const QModelIndex &p) const override
    {
        return p.isValid() ? 1 : Super::NCols;
    }

    bool hasChildren(const QModelIndex &p) const override
    {
        if(p.isValid())
        {
            if(p.internalPointer() || p.row() >= m_data.topLevelCount())
                return false;

            //Every tree has always childrens otherwhise gets removed
            return true;
        }

        return m_data.topLevelCount() > 0;
    }

    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override
    {
        if(!idx.isValid())
            return QModelIndex();

        if(idx.internalPointer())
        {
            //It's a row inside a toplevel tree
            if(column >= Super::NCols)
                return QModelIndex();

            void *ptr = idx.internalPointer();
            if(row != idx.row())
            {
                //Calculate new ptr for row
                RowData *item = static_cast<RowData*>(idx.internalPointer());
                auto topLevel = m_data.getParent(item);
                if(!topLevel || row >= topLevel->childCount())
                    return QModelIndex(); //Out of bound child row
                ptr = const_cast<void *>(static_cast<const void *>(topLevel->ptrForRow(row)));
            }

            return createIndex(row, column, ptr);
        }

        if(column > 0 || row >= m_data.topLevelCount())
            return QModelIndex(); //Parents (RS Items) have only column zero

        return createIndex(row, 0, nullptr);
    }

    inline const RowData *getItem(const QModelIndex& idx) const { return static_cast<const RowData*>(idx.internalPointer()); }

protected:
    ModelData m_data;
};

#endif // SINGLEDEPTHREEMODELHELPER_H
