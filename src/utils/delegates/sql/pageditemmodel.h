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

#ifndef PAGEDITEMMODEL_H
#define PAGEDITEMMODEL_H

#include <QAbstractTableModel>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

/*!
 * \brief The IPagedItemModel class
 *
 * A convinience class to build paged item models
 */
class IPagedItemModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    IPagedItemModel(const int itemsPerPage, sqlite3pp::database &db, QObject *parent = nullptr);

    inline sqlite3pp::database &getDb() const
    {
        return mDb;
    }

    // Cached rows management
    virtual void clearCache() = 0;
    virtual void refreshData(bool forceUpdate = false);

    // Sorting
    virtual void setSortingColumn(int col);
    int getSortingColumn() const;

    // Items
    qint64 getTotalItemsCount();

    // Pages
    int getPageCount();
    int currentPage();
    void switchToPage(int page);

    // Filter

    /*!
     * \brief nullFilterStr
     *
     * String to enter into the filter to get NULL cells
     * \sa FilterFlag
     */
    static constexpr QLatin1String nullFilterStr = QLatin1String("#NULL", 5);

    /*!
     * \brief The FilterFlag enum
     *
     * This enums represent the filtering operations supported on a given columns
     *
     * \sa getFilterAtCol()
     */
    enum FilterFlag
    {
        NoFiltering    = 0, /*!< This column doesn't support filtering, do not show filter */
        BasicFiltering = 1, /*!< This column supports filtering, show filter */
        ExplicitNULL =
          (1 << 1) /*!< This column can be filtered with "\#NULL" string \sa nullFilterStr */
    };

    typedef QFlags<FilterFlag> FilterFlags;

    /*!
     * \brief getFilterAtCol
     * \param col the column requested
     * \return a pair of filter string and filter options
     *
     * This functions returns the state of the filter for a given column
     * Columns that do not support filtering must return an empty filter string
     *
     * \sa setFilterAtCol()
     * \sa FilterFlag
     * \sa FilterHeaderView
     */
    virtual std::pair<QString, FilterFlags> getFilterAtCol(int col);

    /*!
     * \brief setFilterAtCol
     * \param col the column requested
     * \param str filter string to set
     * \return true on success
     *
     * Sets the filter if requsted column supports filtering
     * Remember to call \ref refreshData() to update model
     *
     * \sa refreshData()
     * \sa FilterHeaderView
     */
    virtual bool setFilterAtCol(int col, const QString &str);

signals:
    void modelError(const QString &msg);

    // Items signals
    void totalItemsCountChanged(qint64 count);
    void itemsReady(int startRow, int endRow); // In local indices

    // Page signals
    void pageCountChanged(int count);
    void currentPageChanged(int page);

    // Filter signals
    void filterChanged();

public:
    void clearCache_slot();

protected:
    virtual qint64 recalcTotalItemCount();

protected:
    sqlite3pp::database &mDb;
    qint64 totalItemsCount;
    int curItemCount;
    int pageCount;
    int curPage;
    int sortColumn;
    const int ItemsPerPage;
};

#endif // PAGEDITEMMODEL_H
