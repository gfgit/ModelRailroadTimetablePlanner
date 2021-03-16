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
    virtual void refreshData() = 0;

    // Sorting TODO: enable multiple columns sort/filter with custom QHeaderView
    virtual void setSortingColumn(int col) = 0;
    int getSortingColumn() const;

    // Items
    qint64 getTotalItemsCount();

    // Pages
    int getPageCount();
    int currentPage();
    void switchToPage(int page);

signals:
    void modelError(const QString& msg);

    // Items signals
    void totalItemsCountChanged(qint64 count);
    void itemsReady(int startRow, int endRow); //In local indices

    // Page signals
    void pageCountChanged(int count);
    void currentPageChanged(int page);

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
