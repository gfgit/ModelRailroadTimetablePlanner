#include "rsimportedrollingstockmodel.h"

#include "../rsimportstrings.h"

#include "utils/rs_utils.h"

#include <QEvent>
#include "utils/worker_event_types.h"

#include <QCoreApplication>

#include <QBrush>

#include <QDebug>

class RSResultEvent : public QEvent
{
public:
    static constexpr Type _Type = Type(CustomEvents::RsImportedRSModelResult);
    inline RSResultEvent() : QEvent(_Type) {}

    QVector<RSImportedRollingstockModel::RSItem> items;
    int firstRow;
};

RSImportedRollingstockModel::RSImportedRollingstockModel(database &db, QObject *parent) :
    IRsImportModel(db, parent),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize)
{
    sortColumn = Owner;
}

bool RSImportedRollingstockModel::event(QEvent *e)
{
    if(e->type() == RSResultEvent::_Type)
    {
        RSResultEvent *ev = static_cast<RSResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return QAbstractTableModel::event(e);
}

void RSImportedRollingstockModel::fetchRow(int row)
{
    if(row >= firstPendingRow && row < firstPendingRow + BatchSize)
        return; //Already fetching

    if(row >= cacheFirstRow && row < cacheFirstRow + cache.size())
        return; //Already cached

    //TODO: abort fetching here

    const int remainder = row % BatchSize;
    firstPendingRow = row - remainder;
    qDebug() << "Requested:" << row << "From:" << firstPendingRow;

    QVariant val;
    int valRow = 0;
    RSItem *item = nullptr;

    if(cache.size())
    {
        if(firstPendingRow >= cacheFirstRow + cache.size())
        {
            valRow = cacheFirstRow + cache.size();
            item = &cache.last();
        }
        else if(firstPendingRow > (cacheFirstRow - firstPendingRow))
        {
            valRow = cacheFirstRow; //It's shortet to get here by reverse from cache first
            item = &cache.first();
        }
    }

    switch (sortColumn)
    {
    case Model:
    {
        if(item)
        {
            val = item->modelName;
        }
        break;
    }
    case Number:
    {
        if(item)
        {
            val = item->number;
        }
        break;
    }
    case Owner:
    {
        if(item)
        {
            val = item->ownerName;
        }
        break;
    }
    }

    //TODO: use a custom QRunnable
    QMetaObject::invokeMethod(this, "internalFetch", Qt::QueuedConnection,
                              Q_ARG(int, firstPendingRow), Q_ARG(int, sortColumn),
                              Q_ARG(int, valRow), Q_ARG(QVariant, val));
}

void RSImportedRollingstockModel::internalFetch(int first, int sortCol, int valRow, const QVariant& val)
{
    query q(mDb);

    int offset = first - valRow;
    bool reverse = false;

    if(valRow > first)
    {
        offset = 0;
        reverse = true;
    }

    qDebug() << "Fetching:" << first << "ValRow:" << valRow << val << "Offset:" << offset << "Reverse:" << reverse;

    const char *whereCol;

    QByteArray sql = "SELECT imp.id, imp.import, imp.model_id, imp.owner_id, imp.number, imp.new_number, models.name, models.type, owners.name, owners.new_name"
                     " FROM imported_rs_list imp"
                     " JOIN imported_rs_models models ON models.id=imp.model_id"
                     " JOIN imported_rs_owners owners ON owners.id=imp.owner_id";
    switch (sortCol)
    {
    case Import:
    {
        whereCol = "imp.import";
        break;
    }
    case Model:
    {
        whereCol = "models.name";
        break;
    }
    case Number:
    {
        whereCol = "imp.number";
        break;
    }
    case Owner:
    {
        whereCol = "owners.name";
        break;
    }
    }

    sql += " WHERE owners.import=1 AND models.import=1";
    if(val.isValid())
    {
        sql += " AND ";
        sql += whereCol;
        if(reverse)
            sql += "<?3";
        else
            sql += ">?3";
    }

    sql += " ORDER BY ";
    sql += whereCol;

    if(reverse)
        sql += " DESC";

    sql += " LIMIT ?1";
    if(offset)
        sql += " OFFSET ?2";

    q.prepare(sql);
    sqlite3_stmt *st = q.stmt();
    q.bind(1, BatchSize);
    if(offset)
        q.bind(2, offset);

    if(val.isValid())
    {
        switch (sortCol)
        {
        case Model:
        case Owner:
        {
            QByteArray name = val.toByteArray();
            sqlite3_bind_text(st, 3, name, name.size(), SQLITE_TRANSIENT);
            break;
        }
        case Number:
        {
            int num = val.toInt();
            q.bind(3, num);
            break;
        }
        }
    }

    QVector<RSItem> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    if(reverse)
    {
        int i = BatchSize - 1;

        for(; it != end; ++it)
        {
            auto r = *it;
            RSItem &item = vec[i];
            item.importdRsId = r.get<db_id>(0);
            item.import = r.get<int>(1) == 1;
            item.importedModelId = r.get<db_id>(2);
            item.importedOwnerId = r.get<db_id>(3);

            item.number = r.get<int>(4);

            if(r.column_type(5) == SQLITE_NULL)
                item.new_number = -1;
            else
                item.new_number = r.get<int>(5);

            item.modelName = QByteArray(reinterpret_cast<char const*>(sqlite3_column_text(st, 6)),
                                        sqlite3_column_bytes(st, 6));
            item.type = RsType(r.get<int>(7));

            item.ownerName = QByteArray(reinterpret_cast<char const*>(sqlite3_column_text(st, 8)),
                                        sqlite3_column_bytes(st, 8));

            if(r.column_type(8) != SQLITE_NULL)
            {
                item.ownerCustomName = QByteArray(reinterpret_cast<char const*>(sqlite3_column_text(st, 9)),
                                                  sqlite3_column_bytes(st, 9));
            }
            i--;
        }
        if(i > -1)
            vec.remove(0, i + 1);
    }
    else
    {
        int i = 0;

        for(; it != end; ++it)
        {
            auto r = *it;
            RSItem &item = vec[i];
            item.importdRsId = r.get<db_id>(0);
            item.import = r.get<int>(1) == 1;
            item.importedModelId = r.get<db_id>(2);
            item.importedOwnerId = r.get<db_id>(3);

            item.number = r.get<int>(4);

            if(r.column_type(5) == SQLITE_NULL)
                item.new_number = -1;
            else
                item.new_number = r.get<int>(5);

            item.modelName = QByteArray(reinterpret_cast<char const*>(sqlite3_column_text(st, 6)),
                                        sqlite3_column_bytes(st, 6));
            item.type = RsType(r.get<int>(7));

            item.ownerName = QByteArray(reinterpret_cast<char const*>(sqlite3_column_text(st, 8)),
                                        sqlite3_column_bytes(st, 8));

            if(r.column_type(8) != SQLITE_NULL)
            {
                item.ownerCustomName = QByteArray(reinterpret_cast<char const*>(sqlite3_column_text(st, 9)),
                                                  sqlite3_column_bytes(st, 9));
            }
            i++;
        }
        if(i < BatchSize)
            vec.remove(i, BatchSize - i);
    }


    RSResultEvent *ev = new RSResultEvent;
    ev->items = vec;
    ev->firstRow = first;

    qApp->postEvent(this, ev);
}

void RSImportedRollingstockModel::handleResult(const QVector<RSItem>& items, int firstRow)
{
    if(firstRow == cacheFirstRow + cache.size())
    {
        qDebug() << "RES: appending First:" << cacheFirstRow;
        cache.append(items);
        if(cache.size() > ItemsPerPage)
        {
            const int extra = cache.size() - ItemsPerPage; //Round up to BatchSize
            const int remainder = extra % BatchSize;
            const int n = remainder ? extra + BatchSize - remainder : extra;
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
            QVector<RSItem> tmp = items;
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

    firstPendingRow = -BatchSize;

    QModelIndex firstIdx = index(firstRow, 0);
    QModelIndex lastIdx = index(firstRow + items.count() - 1, NCols - 1);
    emit dataChanged(firstIdx, lastIdx);

    qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}

/* QAbstractTableModel */

QVariant RSImportedRollingstockModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case Import:
            return RsImportStrings::tr("Import");
        case Model:
            return RsImportStrings::tr("Model");
        case Number:
            return RsImportStrings::tr("Number");
        case Owner:
            return RsImportStrings::tr("Owner");
        case NewNumber:
            return RsImportStrings::tr("New number");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

int RSImportedRollingstockModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int RSImportedRollingstockModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant RSImportedRollingstockModel::data(const QModelIndex &idx, int role) const
{
    int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    //qDebug() << "Data:" << idx.row();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<RSImportedRollingstockModel *>(this)->fetchRow(row);

        //Temporarily return null
        return QVariant("...");
    }

    auto item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case Model:
            return item.modelName;
        case Number:
            return rs_utils::formatNum(item.type, item.number);
        case Owner:
            if(item.ownerCustomName.isEmpty())
                return item.ownerName;
            return item.ownerName + " (" + item.ownerCustomName + ')';
        case NewNumber:
            return item.new_number == -1 ? QVariant() : rs_utils::formatNum(item.type, item.new_number);
        }
        break;
    }
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case NewNumber:
            return item.new_number;
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        if(idx.column() == Number || idx.column() == NewNumber)
            return Qt::AlignRight + Qt::AlignVCenter;
        break;
    }
    case Qt::BackgroundRole:
    {
        if(!item.import || (idx.column() == NewNumber && item.new_number == -1))
            return QBrush(Qt::lightGray); //If not imported mark background or no custom number set
        break;
    }
    case Qt::CheckStateRole:
    {
        switch (idx.column())
        {
        case Import:
            return item.import ? Qt::Checked : Qt::Unchecked;
        case NewNumber:
            if(item.new_number == -1)
                return QVariant();
            return Qt::Checked;
        }
        break;
    }
    }

    return QVariant();
}

bool RSImportedRollingstockModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    const int row = idx.row();
    if(!idx.isValid() || row >= curItemCount || idx.column() >= NCols || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    RSItem &item = cache[row - cacheFirstRow];

    QModelIndex first = idx;
    QModelIndex last = idx;

    switch (role)
    {
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case NewNumber:
        {
            int newNumber = value.toInt() % 10000;

            if(item.new_number == newNumber)
                return false;

            QString errText;
            if(!checkNewNumberIsValid(item.importdRsId, item.importedModelId, item.importedModelMatchId,
                                      item.type, item.number, newNumber, &errText))
            {
                emit modelError(errText);
                return false;
            }

            command set_newNumber(mDb, "UPDATE imported_rs_list SET new_number=? WHERE id=?");
            if(newNumber == -1)
                set_newNumber.bind(1); //Bind NULL
            else
                set_newNumber.bind(1, newNumber);
            set_newNumber.bind(2, item.importdRsId);
            if(set_newNumber.execute() != SQLITE_OK)
                return false;

            item.new_number = newNumber;

            break;
        }
        default:
            return false;
        }
        break;
    }
    case Qt::CheckStateRole:
    {
        switch (idx.column())
        {
        case Import:
        {
            Qt::CheckState cs = value.value<Qt::CheckState>();
            const bool import = cs == Qt::Checked;
            if(item.import == import)
                return false; //No change

            if(import)
            {
                //Newly imported, check if there are duplicates
                QString errText;
                if(!checkNewNumberIsValid(item.importdRsId, item.importedModelId, item.importedModelMatchId,
                                          item.type, item.number, item.new_number, &errText))
                {
                    emit modelError(errText);
                    return false;
                }
            }

            command set_imported(mDb, "UPDATE imported_rs_list SET import=? WHERE id=?");
            set_imported.bind(1, import ? 1 : 0);
            set_imported.bind(2, item.importdRsId);
            if(set_imported.execute() != SQLITE_OK)
                return false;

            item.import = import;

            if(sortColumn == Import)
            {
                //This row has now changed position so we need to invalidate cache
                //HACK: we emit dataChanged for this index (that doesn't exist anymore)
                //but the view will trigger fetching at same scroll position so it is enough
                cache.clear();
                cacheFirstRow = 0;
            }

            emit importCountChanged();

            //Update all columns to update background
            first = index(row, 0);
            last = index(row, NCols - 1);
            break;
        }
        case NewNumber:
        {
            Qt::CheckState cs = value.value<Qt::CheckState>();
            if(cs == Qt::Unchecked)
                return setData(idx, -1, Qt::EditRole); //Set -1 as new_number -> NULL
            return false;
        }
        default:
            return false;
        }
        break;
    }
    default:
        return false;
    }

    emit dataChanged(first, last);
    return true;
}

Qt::ItemFlags RSImportedRollingstockModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f; //Not fetched yet

    if(idx.column() == Import)
        f.setFlag(Qt::ItemIsUserCheckable);
    if(idx.column() == NewNumber)
    {
        f.setFlag(Qt::ItemIsEditable);
        f.setFlag(Qt::ItemIsUserCheckable, cache.at(idx.row() - cacheFirstRow).new_number != -1);
    }

    return f;
}

/* ISqlOnDemandModel */

qint64 RSImportedRollingstockModel::recalcTotalItemCount()
{
    query q(mDb, "SELECT COUNT(1) FROM imported_rs_list imp"
                 " JOIN imported_rs_models m ON m.id=imp.model_id"
                 " JOIN imported_rs_owners o ON o.id=imp.owner_id"
                 " WHERE o.import=1 AND m.import=1");
    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

void RSImportedRollingstockModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void RSImportedRollingstockModel::setSortingColumn(int col)
{
    if(sortColumn == col || col == NewNumber)
        return; //Don't sort by NewNumber because some are NULL

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

/* IRsImportModel */

int RSImportedRollingstockModel::countImported()
{
    query q(mDb, "SELECT COUNT(1) FROM imported_rs_list imp"
                 " JOIN imported_rs_models m ON m.id=imp.model_id"
                 " JOIN imported_rs_owners o ON o.id=imp.owner_id"
                 " WHERE imp.import=1 AND o.import=1 AND m.import=1");
    q.step();
    const int count = q.getRows().get<int>(0);
    return count;
}

/* ICheckName */

bool RSImportedRollingstockModel::checkNewNumberIsValid(db_id importedRsId, db_id importedModelId, db_id matchExistingModelId,
                                                        RsType rsType, int number, int newNumber, QString *errTextOut)
{
    RsType type = RsType(rsType);

    if(number == newNumber)
    {
        if(errTextOut)
        {
            *errTextOut = tr("You cannot set the same name in the 'Custom Name' field.\n"
                             "If you meant to revert to original name then clear the custom name and leave the cell empty");
        }
        return false;
    }

    int numberToCheck = newNumber;
    if(newNumber == -1)
        numberToCheck = number;

    //First check if there is an imported RS with same number or new number
    query q(mDb, "SELECT imp.id, imp.new_number, m.name, m.new_name, rs_models.name"
                 " FROM imported_rs_models m"
                 " LEFT JOIN rs_models ON rs_models.id=m.match_existing_id"
                 " JOIN imported_rs_list imp ON imp.model_id=m.id"
                 " JOIN imported_rs_owners own ON own.id=imp.owner_id"
                 " WHERE (m.id=?1 OR (m.match_existing_id=?2 AND m.match_existing_id NOT NULL))"
                 " AND imp.id<>?3 AND imp.import=1 AND m.import=1 AND own.import=1"
                 " AND ((imp.new_number IS NULL AND imp.number=?4) OR imp.new_number=?4)");
    q.bind(1, importedModelId);
    q.bind(2, matchExistingModelId);
    q.bind(3, importedRsId);
    q.bind(4, numberToCheck);

    if(q.step() == SQLITE_ROW)
    {
        auto r = q.getRows();
        db_id dupId = r.get<db_id>(0);
        Q_UNUSED(dupId) //TODO: maybe use it?

        sqlite3_stmt *st = q.stmt();

        bool matchedNewNumber = true;

        if(r.column_type(1) == SQLITE_NULL)
        {
            matchedNewNumber = false; //We matched original number
        }

        QByteArray modelName = QByteArray(reinterpret_cast<char const*>(sqlite3_column_text(st, 2)),
                                          sqlite3_column_bytes(st, 2));

        if(errTextOut)
        {
            if(r.column_type(3) != SQLITE_NULL)
            {
                // 'name (new_name)'
                modelName.append(" (", 2);
                modelName.append(reinterpret_cast<char const*>(sqlite3_column_text(st, 3)),
                                 sqlite3_column_bytes(st, 3));
                modelName.append(')');
            }

            if(r.column_type(4) != SQLITE_NULL)
            {
                // 'name (match_existing name)'
                modelName.append(" (", 2);
                modelName.append(reinterpret_cast<char const*>(sqlite3_column_text(st, 4)),
                                 sqlite3_column_bytes(st, 4));
                modelName.append(')');
            }

            QString model = QString::fromUtf8(modelName);

            if(matchedNewNumber)
            {
                *errTextOut = tr("There is already another imported rollingstock with same 'New Number': <b>%1 %2</b>")
                        .arg(model).arg(rs_utils::formatNum(type, numberToCheck));
            }else{
                *errTextOut = tr("There is already another imported rollingstock with same number: <b>%1 %2</b>")
                        .arg(model).arg(rs_utils::formatNum(type, numberToCheck));
            }
        }
        return false;
    }

    //Then check for an existing RS with same number if model is matched
    if(matchExistingModelId)
    {
        q.prepare("SELECT rs_list.id, rs_models.name"
                  " FROM rs_list"
                  " JOIN rs_models ON rs_models.id=?1"
                  " WHERE rs_list.model_id=?1 AND rs_list.number=?");
        q.bind(1, matchExistingModelId);
        q.bind(2, numberToCheck);

        if(q.step() == SQLITE_ROW)
        {
            auto r = q.getRows();
            db_id dupExistingId = r.get<db_id>(0);
            Q_UNUSED(dupExistingId) //TODO: maybe use it?

            QString modelName = r.get<QString>(1);

            if(errTextOut)
            {
                *errTextOut = tr("There is already an existing rollingstock with same number: <b>%1 %2</b>")
                        .arg(modelName).arg(rs_utils::formatNum(type, numberToCheck));
            }
            return false;
        }
    }

    return true;
}
