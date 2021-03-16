#include "rslistondemandmodel.h"
#include "rslistondemandmodelresultevent.h"

#include <QFont>

RSListOnDemandModel::RSListOnDemandModel(sqlite3pp::database &db, QObject *parent) :
    IPagedItemModel(50, db, parent),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize)
{
    sortColumn = Name;
}

bool RSListOnDemandModel::event(QEvent *e)
{
    if(e->type() == RSListOnDemandModelResultEvent::_Type)
    {
        RSListOnDemandModelResultEvent *ev = static_cast<RSListOnDemandModelResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return QAbstractTableModel::event(e);
}

int RSListOnDemandModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int RSListOnDemandModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant RSListOnDemandModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    //qDebug() << "Data:" << idx.row();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<RSListOnDemandModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const RSItem& item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case Name:
            return item.name;
        }

        break;
    }
    case Qt::FontRole:
    {
        if(item.type == RsType::Engine)
        {
            //Engines in bold
            QFont f;
            f.setBold(true);
            return f;
        }
        break;
    }
    }

    return QVariant();
}

void RSListOnDemandModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void RSListOnDemandModel::fetchRow(int row)
{
    if(firstPendingRow != -BatchSize)
        return; //Currently fetching another batch, wait for it to finish first

    if(row >= firstPendingRow && row < firstPendingRow + BatchSize)
        return; //Already fetching this batch

    if(row >= cacheFirstRow && row < cacheFirstRow + cache.size())
        return; //Already cached

    //TODO: abort fetching here

    const int remainder = row % BatchSize;
    firstPendingRow = row - remainder;

    QVariant val;
    int valRow = 0;
//    RSItem *item = nullptr;

//    if(cache.size())
//    {
//        if(firstPendingRow >= cacheFirstRow + cache.size())
//        {
//            valRow = cacheFirstRow + cache.size();
//            item = &cache.last();
//        }
//        else if(firstPendingRow > (cacheFirstRow - firstPendingRow))
//        {
//            valRow = cacheFirstRow;
//            item = &cache.first();
//        }
//    }

    /*switch (sortCol) TODO: use val in WHERE clause
    {
    case Name:
    {
        if(item)
        {
            val = item->name;
        }
        break;
    }
        //No data hint for TypeCol column
    }*/

    //TODO: use a custom QRunnable
    //    QMetaObject::invokeMethod(this, "internalFetch", Qt::QueuedConnection,
    //                              Q_ARG(int, firstPendingRow), Q_ARG(int, sortCol),
    //                              Q_ARG(int, valRow), Q_ARG(QVariant, val));
    internalFetch(firstPendingRow, sortColumn, val.isNull() ? 0 : valRow, val);
}

void RSListOnDemandModel::handleResult(const QVector<RSItem>& items, int firstRow)
{
    if(firstRow == cacheFirstRow + cache.size())
    {
        cache.append(items);
        if(cache.size() > ItemsPerPage)
        {
            const int extra = cache.size() - ItemsPerPage; //Round up to BatchSize
            const int remainder = extra % BatchSize;
            const int n = remainder ? extra + BatchSize - remainder : extra;
            cache.remove(0, n);
            cacheFirstRow += n;
        }
    }
    else
    {
        if(firstRow + items.size() == cacheFirstRow)
        {
            QVector<RSItem> tmp = items;
            tmp.append(cache);
            cache = tmp;
            if(cache.size() > ItemsPerPage)
            {
                const int n = cache.size() - ItemsPerPage;
                cache.remove(ItemsPerPage, n);
            }
        }
        else
        {
            cache = items;
        }
        cacheFirstRow = firstRow;
    }

    firstPendingRow = -BatchSize;

    int lastRow = firstRow + items.count(); //Last row + 1 extra to re-trigger possible next batch
    if(lastRow >= curItemCount)
        lastRow = curItemCount -1; //Ok, there is no extra row so notify just our batch

    if(firstRow > 0)
        firstRow--; //Try notify also the row before because there might be another batch waiting so re-trigger it
    QModelIndex firstIdx = index(firstRow, 0);
    QModelIndex lastIdx = index(lastRow, NCols - 1);
    emit dataChanged(firstIdx, lastIdx);
}

void RSListOnDemandModel::setSortingColumn(int /*col*/)
{
    //Only sort by name
}
