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

#ifndef IPAGEDITEMMODELHELPER_H
#define IPAGEDITEMMODELHELPER_H

#include "pageditemmodel.h"

#include <QEvent>

/*!
 * \brief IPagedItemModelImpl common implementation
 *
 * Common implementation for \ref IPagedItemModel to reduce code duplication
 *
 * SuperType is the model inheriting this class
 * ModelItemType is the item to store in cache which represents a signle row
 */
template <typename SuperType, typename ModelItemType>
class IPagedItemModelImpl : public IPagedItemModel
{
protected:
    // Get some definition from SuperType so we don't have to pass them as templates
    static constexpr int NCols_     = SuperType::NCols;
    static constexpr int BatchSize_ = SuperType::BatchSize;

public:
    IPagedItemModelImpl(const int itemsPerPage, sqlite3pp::database &db, QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    // IPagedItemModel

    // Cached rows management
    virtual void clearCache() override;

protected:
    typedef QList<ModelItemType> Cache;

    /*!
     * \brief The ResultEvent class
     *
     * This event will be posted when finished loading a batch of data
     * and will be received by the model so new data will be added to cache
     */
    class ResultEvent : public QEvent
    {
    public:
        static constexpr Type _Type = Type(QEvent::User + 1);
        inline ResultEvent() :
            QEvent(_Type)
        {
        }

        Cache items;
        int firstRow;
    };

    bool event(QEvent *e) override;

protected:
    void fetchRow(int row);
    void postResult(const Cache &items, int firstRow);
    void handleResult(const Cache &items, int firstRow);

protected:
    Cache cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // IPAGEDITEMMODELHELPER_H
