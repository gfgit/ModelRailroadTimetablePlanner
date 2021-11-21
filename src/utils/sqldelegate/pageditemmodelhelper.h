#ifndef IPAGEDITEMMODELHELPER_H
#define IPAGEDITEMMODELHELPER_H

#include "pageditemmodel.h"

#include <QEvent>

template <typename SuperType, typename ModelItemType>
class IPagedItemModelImpl : public IPagedItemModel
{
protected:
    //Get some definition from SuperType so we don't have to pass them as templates
    static constexpr int NCols_ = SuperType::NCols;
    static constexpr int BatchSize_ = SuperType::BatchSize;

public:
    IPagedItemModelImpl(const int itemsPerPage, sqlite3pp::database &db, QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    // IPagedItemModel

    // Cached rows management
    virtual void clearCache() override;

protected:
    typedef QVector<ModelItemType> Cache;

    //TODO: this would be better in .cpp file so
    //we do not have to include <QEvent> here
    class ResultEvent : public QEvent
    {
    public:
        static constexpr Type _Type = Type(QEvent::User + 1);
        inline ResultEvent() : QEvent(_Type) {}

        Cache items;
        int firstRow;
    };

    bool event(QEvent *e) override;

protected:
    void fetchRow(int row);
    void postResult(const Cache &items, int firstRow);
    void handleResult(const Cache &items, int firstRow);

protected:
    Cache cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // IPAGEDITEMMODELHELPER_H
