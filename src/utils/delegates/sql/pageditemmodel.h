#ifndef PAGEDITEMMODEL_H
#define PAGEDITEMMODEL_H

#include <QAbstractTableModel>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class IPagedItemModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    /* Full index: represents the index which the item would have in the table ordered by sorting rules
     * Local index: is the row of the item inside the page (valid only for the page of the item)
     */


    IPagedItemModel(const int itemsPerPage, sqlite3pp::database &db, QObject *parent = nullptr);

    inline sqlite3pp::database &getDb() const { return mDb; }

    // Cached rows management
    virtual void clearCache() = 0;
    virtual void refreshData(bool forceUpdate = false);

    // Sorting TODO: enable multiple columns sort/filter with custom QHeaderView
    virtual void setSortingColumn(int col);
    int getSortingColumn() const;

    // Items
    qint64 getTotalItemsCount();

    // Pages
    int getPageCount();
    int currentPage();
    void switchToPage(int page);

    //Filter
    static constexpr QLatin1String nullFilterStr = QLatin1String("#NULL", 5);

    enum FilterFlag
    {
        NoFiltering = 0,
        BasicFiltering = 1,
        ExplicitNULL = (1<<1)
    };

    typedef QFlags<FilterFlag> FilterFlags;

    virtual std::pair<QString, FilterFlags> getFilterAtCol(int col);
    virtual bool setFilterAtCol(int col, const QString& str);

signals:
    void modelError(const QString& msg);

    // Items signals
    void totalItemsCountChanged(qint64 count);
    void itemsReady(int startRow, int endRow); //In local indices

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
