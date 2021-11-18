#ifndef IPAGEDITEMMODELHELPER_H
#define IPAGEDITEMMODELHELPER_H

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

#endif // IPAGEDITEMMODELHELPER_H
