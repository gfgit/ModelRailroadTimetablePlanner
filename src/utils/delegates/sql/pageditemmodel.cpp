#include "pageditemmodel.h"

#include <sqlite3pp/sqlite3pp.h>

IPagedItemModel::IPagedItemModel(const int itemsPerPage, sqlite3pp::database &db, QObject *parent) :
    QAbstractTableModel(parent),
    mDb(db),
    totalItemsCount(0),
    curItemCount(0),
    pageCount(0),
    curPage(0),
    sortColumn(0),
    ItemsPerPage(itemsPerPage)
{

}

void IPagedItemModel::refreshData(bool forceUpdate)
{
    if(!mDb.db())
        return;

    emit itemsReady(-1, -1); //Notify we are about to refresh

    qint64 count = recalcTotalItemCount();
    if(count != totalItemsCount || forceUpdate)
    {
        beginResetModel();

        clearCache();
        totalItemsCount = count;
        emit totalItemsCountChanged(totalItemsCount);

        //Round up division
        const int rem = count % ItemsPerPage;
        pageCount = count / ItemsPerPage + (rem != 0);
        emit pageCountChanged(pageCount);

        if(curPage >= pageCount)
        {
            switchToPage(pageCount - 1);
        }

        curItemCount = totalItemsCount ? (curPage == pageCount - 1 && rem) ? rem : ItemsPerPage : 0;

        endResetModel();
    }
}

void IPagedItemModel::setSortingColumn(int col)
{
    //Do nothing, it must be reimplemented
    Q_UNUSED(col)
}

int IPagedItemModel::getSortingColumn() const
{
    return sortColumn;
}

qint64 IPagedItemModel::getTotalItemsCount()
{
    return totalItemsCount;
}

int IPagedItemModel::getPageCount()
{
    return pageCount;
}

int IPagedItemModel::currentPage()
{
    return curPage;
}

void IPagedItemModel::switchToPage(int page)
{
    if(curPage == page || page < 0 || page >= pageCount)
        return;

    clearCache();
    curPage = page;

    const int rem = totalItemsCount % ItemsPerPage;
    const int items = (curPage == pageCount - 1 && rem) ? rem : ItemsPerPage;
    if(items != curItemCount)
    {
        beginResetModel();
        curItemCount = items;
        endResetModel();
    }

    emit currentPageChanged(curPage);

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, columnCount() - 1);
    emit dataChanged(first, last);
}

std::pair<QString, IPagedItemModel::FilterFlags> IPagedItemModel::getFilterAtCol(int col)
{
    Q_UNUSED(col)
    return {QString(), FilterFlag::NoFiltering};
}

bool IPagedItemModel::setFilterAtCol(int col, const QString &str)
{
    Q_UNUSED(col)
    Q_UNUSED(str)
    return false;
}

void IPagedItemModel::clearCache_slot()
{
    clearCache();
    QModelIndex start = index(0, 0);
    QModelIndex end = index(rowCount() - 1, columnCount() - 1);
    emit dataChanged(start, end);
}

qint64 IPagedItemModel::recalcTotalItemCount()
{
    //NOTE: either override this or refreshData()
    return 0; //Default implementation
}
