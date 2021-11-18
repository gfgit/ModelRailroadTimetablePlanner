#ifndef IPAGEDITEMMODELIMPL_H
#define IPAGEDITEMMODELIMPL_H

#include "pageditemmodel.h"

#include <QEvent>

#include <QDebug>

template <typename SuperType>
class IPagedItemModelImpl : public IPagedItemModel
{
private:
    //Get some definition from SuperType so we don't have to pass them as templates
    typedef typename SuperType::ModelItemType ModelItemType_;
    static constexpr int NCols_ = SuperType::NCols;
    static constexpr int BatchSize_ = SuperType::BatchSize;
public:
    IPagedItemModelImpl(const int itemsPerPage, sqlite3pp::database &db, QObject *parent = nullptr)
        : IPagedItemModel(itemsPerPage, db, parent),
        cacheFirstRow(0),
        firstPendingRow(-BatchSize_)
    {
    }

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    // IPagedItemModel

    // Cached rows management
    virtual void clearCache() override;

protected:

    //TODO: this would be better in .cpp file so
    //we do not have to include <QEvent> here
    class ResultEvent : public QEvent
    {
        static constexpr Type _Type = Type(QEvent::User + 1);
        inline ResultEvent() : QEvent(_Type) {}

        QVector<ModelItemType_> items;
        int firstRow;
    };

    bool event(QEvent *e) override;

protected:
    void fetchRow(int row);
    void handleResult(const QVector<ModelItemType_> &items, int firstRow);

protected:
    QVector<ModelItemType_> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

template<typename SuperType>
int IPagedItemModelImpl<SuperType>::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

template<typename SuperType>
int IPagedItemModelImpl<SuperType>::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols_;
}

template<typename SuperType>
void IPagedItemModelImpl<SuperType>::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

template<typename SuperType>
bool IPagedItemModelImpl<SuperType>::event(QEvent *e)
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

template<typename SuperType>
void IPagedItemModelImpl<SuperType>::fetchRow(int row)
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

template<typename SuperType>
void IPagedItemModelImpl<SuperType>::handleResult(const QVector<ModelItemType_> &items, int firstRow)
{
    if(firstRow == cacheFirstRow + cache.size())
    {
        qDebug() << "RES: appending First:" << cacheFirstRow;
        cache.append(items);
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
        if(firstRow + items.size() == cacheFirstRow)
        {
            qDebug() << "RES: prepending First:" << cacheFirstRow;
            QVector<ModelItemType_> tmp = items;
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
            cache = items;
        }
        cacheFirstRow = firstRow;
        qDebug() << "NEW First:" << cacheFirstRow;
    }

    firstPendingRow = -BatchSize_; //Tell model we ended fetching

    int lastRow = firstRow + items.count(); //Last row + 1 extra to re-trigger possible next batch
    if(lastRow >= curItemCount)
    {
        //Ok, there is no extra row so notify just our batch
        lastRow = curItemCount -1;
    }
    else if(items.count() < BatchSize_)
    {
        //Error: fetching returned less rows than expected

        //NOTE: when not on last batch of last page, it should always be of BatchSize_
        //      last batch of last page can have less rows

        //Remove the '+1' to avoid triggering fetching of next batch
        //Otherwise it may cause infinite loop.
        emit modelError(IPagedItemModel::tr("Fetching returned less rows than expected.\n"
                                            "%1 instead of %2").arg(items.count()).arg(BatchSize_));
        lastRow--;
    }

    if(firstRow > 0)
        firstRow--; //Try notify also the row before because there might be another batch waiting so re-trigger it
    QModelIndex firstIdx = index(firstRow, 0);
    QModelIndex lastIdx = index(lastRow, NCols_ - 1);
    emit dataChanged(firstIdx, lastIdx);
    emit itemsReady(firstRow, lastRow);

    qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}

#endif // IPAGEDITEMMODELIMPL_H
