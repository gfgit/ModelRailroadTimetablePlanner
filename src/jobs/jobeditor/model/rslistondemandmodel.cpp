#include "rslistondemandmodel.h"

#include "utils/delegates/sql/pageditemmodelhelper_impl.h"

#include <QFont>

RSListOnDemandModel::RSListOnDemandModel(sqlite3pp::database &db, QObject *parent) :
    BaseClass(100, db, parent)
{
    sortColumn = Name;
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
