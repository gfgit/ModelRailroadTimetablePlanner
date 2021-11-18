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
public:
    IPagedItemModelImpl(const int itemsPerPage, sqlite3pp::database &db, QObject *parent = nullptr)
        : IPagedItemModel(itemsPerPage, db, parent),
        cacheFirstRow(0),
        firstPendingRow(-SuperType::BatchSize)
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
void IPagedItemModelImpl<SuperType>::handleResult(const QVector<ModelItemType_> &items, int firstRow)
{
    if(firstRow == cacheFirstRow + cache.size())
    {
        qDebug() << "RES: appending First:" << cacheFirstRow;
        cache.append(items);
        if(cache.size() > ItemsPerPage)
        {
            const int extra = cache.size() - ItemsPerPage; //Round up to BatchSize
            const int remainder = extra % SuperType::BatchSize;
            const int n = remainder ? extra + SuperType::BatchSize - remainder : extra;
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
            if(cache.size() > ItemsPerPage)
            {
                const int n = cache.size() - ItemsPerPage;
                cache.remove(ItemsPerPage, n);
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

    firstPendingRow = -SuperType::BatchSize;

    int lastRow = firstRow + items.count(); //Last row + 1 extra to re-trigger possible next batch
    if(lastRow >= curItemCount)
        lastRow = curItemCount -1; //Ok, there is no extra row so notify just our batch

    if(firstRow > 0)
        firstRow--; //Try notify also the row before because there might be another batch waiting so re-trigger it
    QModelIndex firstIdx = index(firstRow, 0);
    QModelIndex lastIdx = index(lastRow, SuperType::NCols - 1);
    emit dataChanged(firstIdx, lastIdx);
    emit itemsReady(firstRow, lastRow);

    qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}

#endif // IPAGEDITEMMODELIMPL_H
