#include "pageditemmodel.h"

IPagedItemModel::IPagedItemModel(const int itemsPerPage, sqlite3pp::database &db, QObject *parent) :
    QAbstractTableModel(parent),
    mDb(db),
    totalItemsCount(0),
    curItemCount(0),
    pageCount(0),
    curPage(0),
    ItemsPerPage(itemsPerPage)
{

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
