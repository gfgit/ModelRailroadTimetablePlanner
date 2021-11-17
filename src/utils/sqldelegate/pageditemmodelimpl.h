#ifndef IPAGEDITEMMODELIMPL_H
#define IPAGEDITEMMODELIMPL_H

#include "pageditemmodel.h"

#include <QEvent>

template <typename ModelItemType>
class IPagedItemModelImpl : public IPagedItemModel
{
public:
    IPagedItemModelImpl(const int itemsPerPage, sqlite3pp::database &db, QObject *parent = nullptr)
        : IPagedItemModel(itemsPerPage, db, parent)
    {
    }

protected:

    //TODO: this would be better in .cpp file so
    //we do not have to include <QEvent> here
    class ResultEvent : public QEvent
    {
        static constexpr Type _Type = Type(QEvent::User + 1);
        inline ResultEvent() : QEvent(_Type) {}

        QVector<ModelItemType> items;
        int firstRow;
    };

protected:
    QVector<ModelItemType> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // IPAGEDITEMMODELIMPL_H
