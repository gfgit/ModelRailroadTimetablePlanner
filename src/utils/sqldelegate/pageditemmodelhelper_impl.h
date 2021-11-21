#ifndef IPAGEDITEMMODELHELPER_IMPL_H
#define IPAGEDITEMMODELHELPER_IMPL_H

#include "pageditemmodelhelper.h"

#include <QCoreApplication>

#include <QDebug>

template <typename SuperType, typename ModelItemType>
IPagedItemModelImpl<SuperType, ModelItemType>::IPagedItemModelImpl(const int itemsPerPage, sqlite3pp::database &db, QObject *parent)
    : IPagedItemModel(itemsPerPage, db, parent),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize_)
{

}

template <typename SuperType, typename ModelItemType>
int IPagedItemModelImpl<SuperType, ModelItemType>::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

template <typename SuperType, typename ModelItemType>
int IPagedItemModelImpl<SuperType, ModelItemType>::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols_;
}

template <typename SuperType, typename ModelItemType>
void IPagedItemModelImpl<SuperType, ModelItemType>::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

template <typename SuperType, typename ModelItemType>
bool IPagedItemModelImpl<SuperType, ModelItemType>::event(QEvent *e)
{
    if(e->type() == ResultEvent::_Type)
    {
        ResultEvent *ev = static_cast<ResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return IPagedItemModel::event(e);
}

template <typename SuperType, typename ModelItemType>
void IPagedItemModelImpl<SuperType, ModelItemType>::fetchRow(int row)
{
    if(firstPendingRow != -BatchSize_)
        return; //Currently fetching another batch, wait for it to finish first

    if(row >= firstPendingRow && row < firstPendingRow + BatchSize_)
        return; //Already fetching this batch

    if(row >= cacheFirstRow && row < cacheFirstRow + cache.size())
        return; //Already cached

    //TODO: abort previous fetching here

    const int remainder = row % BatchSize_;
    firstPendingRow = row - remainder;
    qDebug() << "Requested:" << row << "From:" << firstPendingRow;

    QVariant val;
    int valRow = 0;


    //TODO: use a custom QRunnable
    //    QMetaObject::invokeMethod(this, "internalFetch", Qt::QueuedConnection,
    //                              Q_ARG(int, firstPendingRow), Q_ARG(int, sortCol),
    //                              Q_ARG(int, valRow), Q_ARG(QVariant, val));
    SuperType *self = static_cast<SuperType *>(this);
    self->internalFetch(firstPendingRow, sortColumn, val.isNull() ? 0 : valRow, val);
}

template<typename SuperType, typename ModelItemType>
void IPagedItemModelImpl<SuperType, ModelItemType>::postResult(const Cache &items, int firstRow)
{
    ResultEvent *ev = new ResultEvent;
    ev->items = items;
    ev->firstRow = firstRow;

    qApp->postEvent(this, ev);
}

template <typename SuperType, typename ModelItemType>
void IPagedItemModelImpl<SuperType, ModelItemType>::handleResult(const Cache &items, int firstRow)
{
    Cache itemsCopy = items;

    int lastRow = firstRow + itemsCopy.count(); //Last row + 1 extra to re-trigger possible next batch
    if(lastRow >= curItemCount)
    {
        //Ok, there is no extra row so notify just our batch
        lastRow = curItemCount -1;
    }
    else if(itemsCopy.count() < BatchSize_)
    {
        //Error: fetching returned less rows than expected

        //NOTE: when not on last batch of last page, it should always be of BatchSize_
        //      last batch of last page can have less rows

        emit modelError(IPagedItemModel::tr("Fetching returned less rows than expected.\n"
                                            "%1 instead of %2").arg(itemsCopy.count()).arg(BatchSize_));

        //Insert fake items to prevent triggering re-fetching which leads to infinite loop
        const int expectedCount = qMin(BatchSize_, curItemCount - firstRow);
        const int realCount = itemsCopy.count();
        itemsCopy.insert(realCount, expectedCount - realCount, ModelItemType{});

        lastRow = firstRow + itemsCopy.count() - 1;
    }

    if(firstRow == cacheFirstRow + cache.size())
    {
        qDebug() << "RES: appending First:" << cacheFirstRow;
        cache.append(itemsCopy);
        if(cache.size() > IPagedItemModel::ItemsPerPage)
        {
            const int extra = cache.size() - IPagedItemModel::ItemsPerPage; //Round up to BatchSize
            const int remainder = extra % BatchSize_;
            const int n = remainder ? extra + BatchSize_ - remainder : extra;
            qDebug() << "RES: removing last" << n;
            cache.remove(0, n);
            cacheFirstRow += n;
        }
    }
    else
    {
        if(firstRow + itemsCopy.size() == cacheFirstRow)
        {
            qDebug() << "RES: prepending First:" << cacheFirstRow;
            Cache tmp = itemsCopy;
            tmp.append(cache);
            cache = tmp;
            if(cache.size() > IPagedItemModel::ItemsPerPage)
            {
                const int n = cache.size() - IPagedItemModel::ItemsPerPage;
                cache.remove(IPagedItemModel::ItemsPerPage, n);
                qDebug() << "RES: removing first" << n;
            }
        }
        else
        {
            qDebug() << "RES: replacing";
            cache = itemsCopy;
        }
        cacheFirstRow = firstRow;
        qDebug() << "NEW First:" << cacheFirstRow;
    }

    firstPendingRow = -BatchSize_; //Tell model we ended fetching

    if(firstRow > 0)
        firstRow--; //Try notify also the row before because there might be another batch waiting so re-trigger it
    QModelIndex firstIdx = index(firstRow, 0);
    QModelIndex lastIdx = index(lastRow, NCols_ - 1);
    emit dataChanged(firstIdx, lastIdx);
    emit itemsReady(firstRow, lastRow);

    qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}

#endif // IPAGEDITEMMODELHELPER_IMPL_H


