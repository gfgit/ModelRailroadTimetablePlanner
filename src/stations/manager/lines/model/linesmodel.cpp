#include "linesmodel.h"

#include <QCoreApplication>
#include <QEvent>

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/worker_event_types.h"

#include "utils/kmutils.h"

#include <QDebug>

class LinesModelResultEvent : public QEvent
{
public:
    static constexpr Type _Type = Type(CustomEvents::LinesModelResult);
    inline LinesModelResultEvent() : QEvent(_Type) {}

    QVector<LinesModel::LineItem> items;
    int firstRow;
};

//Error messages
static constexpr char errorNameAlreadyUsedText[] =
    QT_TRANSLATE_NOOP("LinesModel",
                      "The name <b>%1</b> is already used by another line.<br>"
                      "Please choose a different name for each railway line.");

static constexpr char errorLineInUseText[] =
    QT_TRANSLATE_NOOP("LinesModel",
                      "Cannot delete <b>%1</b> line because it is stille referenced.");

LinesModel::LinesModel(sqlite3pp::database &db, QObject *parent) :
    IPagedItemModel(500, db, parent),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize)
{
    sortColumn = NameCol;
}

bool LinesModel::event(QEvent *e)
{
    if(e->type() == LinesModelResultEvent::_Type)
    {
        LinesModelResultEvent *ev = static_cast<LinesModelResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return QAbstractTableModel::event(e);
}

QVariant LinesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            switch (section)
            {
            case NameCol:
                return tr("Name");
            case StartKm:
                return tr("Sart At Km");
            }
            break;
        }
        }
    }
    else if(role == Qt::DisplayRole)
    {
        return section + curPage * ItemsPerPage + 1;
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

int LinesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int LinesModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant LinesModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<LinesModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const LineItem& item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case NameCol:
            return item.name;
        case StartKm:
            return utils::kmNumToText(item.startMeters);
        }
        break;
    }
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case NameCol:
            return item.name;
        case StartKm:
            return item.startMeters;
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        switch (idx.column())
        {
        case StartKm:
            return Qt::AlignRight + Qt::AlignVCenter;
        }
        break;
    }
    }

    return QVariant();
}

void LinesModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void LinesModel::refreshData()
{
    if(!mDb.db())
        return;

    emit itemsReady(-1, -1); //Notify we are about to refresh

    //TODO: consider filters
    query q(mDb, "SELECT COUNT(1) FROM lines");
    q.step();
    const int count = q.getRows().get<int>(0);
    if(count != totalItemsCount)
    {
        beginResetModel();

        clearCache();
        totalItemsCount = count;
        emit totalItemsCountChanged(totalItemsCount);

        //Round up division
        const int rem = count % ItemsPerPage;
        pageCount = count / ItemsPerPage + (rem != 0);
        emit pageCountChanged(pageCount);

        if(curPage >= pageCount)
        {
            switchToPage(pageCount - 1);
        }

        curItemCount = totalItemsCount ? (curPage == pageCount - 1 && rem) ? rem : ItemsPerPage : 0;

        endResetModel();
    }
}

void LinesModel::setSortingColumn(int col)
{
    //Sort only by name
    Q_UNUSED(col)
}

bool LinesModel::addLine(const QString &name, db_id *outLineId)
{
    if(name.isEmpty())
        return false;

    command q_newStation(mDb, "INSERT INTO lines(id,name,start_meters)"
                              " VALUES (NULL, ?, 0)");
    q_newStation.bind(1, name);

    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    int ret = q_newStation.execute();
    db_id stationId = mDb.last_insert_rowid();
    sqlite3_mutex_leave(mutex);
    q_newStation.reset();

    if((ret != SQLITE_OK && ret != SQLITE_DONE) || stationId == 0)
    {
        //Error
        if(outLineId)
            *outLineId = 0;

        if(ret == SQLITE_CONSTRAINT_UNIQUE)
        {
            emit modelError(tr(errorNameAlreadyUsedText).arg(name));
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }
        return false;
    }

    if(outLineId)
        *outLineId = stationId;

    refreshData(); //Recalc row count
    setSortingColumn(NameCol);
    switchToPage(0); //Reset to first page and so it is shown as first row

    return true;
}

bool LinesModel::removeLine(db_id lineId)
{
    command q_removeStation(mDb, "DELETE FROM lines WHERE id=?");

    q_removeStation.bind(1, lineId);
    int ret = q_removeStation.execute();
    q_removeStation.reset();

    if(ret != SQLITE_OK)
    {
        if(ret == SQLITE_CONSTRAINT_TRIGGER)
        {
            //TODO: show more information to the user, like where it's still referenced
            query q(mDb, "SELECT name FROM lines WHERE id=?");
            q.bind(1, lineId);
            if(q.step() == SQLITE_ROW)
            {
                const QString name = q.getRows().get<QString>(0);
                emit modelError(tr(errorLineInUseText).arg(name));
            }
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }

        return false;
    }

    refreshData(); //Recalc row count

    return true;
}

void LinesModel::fetchRow(int row)
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
    qDebug() << "Requested:" << row << "From:" << firstPendingRow;

    QVariant val;
    int valRow = 0;


    //TODO: use a custom QRunnable
    //    QMetaObject::invokeMethod(this, "internalFetch", Qt::QueuedConnection,
    //                              Q_ARG(int, firstPendingRow), Q_ARG(int, sortCol),
    //                              Q_ARG(int, valRow), Q_ARG(QVariant, val));
    internalFetch(firstPendingRow, sortColumn, val.isNull() ? 0 : valRow, val);
}

void LinesModel::internalFetch(int first, int sortCol, int valRow, const QVariant &val)
{
    query q(mDb);

    int offset = first - valRow + curPage * ItemsPerPage;
    bool reverse = false;

    if(valRow > first)
    {
        offset = 0;
        reverse = true;
    }

    qDebug() << "Fetching:" << first << "ValRow:" << valRow << val << "Offset:" << offset << "Reverse:" << reverse;

    const char *whereCol = nullptr;

    QByteArray sql = "SELECT id,name,start_meters FROM lines";
    switch (sortCol)
    {
    case NameCol:
    {
        whereCol = "name"; //Order by 1 column, no where clause
        break;
    }
    }

    if(val.isValid())
    {
        sql += " WHERE ";
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
    q.bind(1, BatchSize);
    if(offset)
        q.bind(2, offset);

    //    if(val.isValid())
    //    {
    //        switch (sortCol)
    //        {
    //        case LineNameCol:
    //        {
    //            q.bind(3, val.toString());
    //            break;
    //        }
    //        }
    //    }

    QVector<LineItem> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    if(reverse)
    {
        int i = BatchSize - 1;

        for(; it != end; ++it)
        {
            auto r = *it;
            LineItem &item = vec[i];
            item.lineId = r.get<db_id>(0);
            item.name = r.get<QString>(1);
            item.startMeters = r.get<int>(2);
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
            LineItem &item = vec[i];
            item.lineId = r.get<db_id>(0);
            item.name = r.get<QString>(1);
            item.startMeters = r.get<int>(2);
            i++;
        }
        if(i < BatchSize)
            vec.remove(i, BatchSize - i);
    }


    LinesModelResultEvent *ev = new LinesModelResultEvent;
    ev->items = vec;
    ev->firstRow = first;

    qApp->postEvent(this, ev);
}

void LinesModel::handleResult(const QVector<LinesModel::LineItem> &items, int firstRow)
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
            QVector<LineItem> tmp = items;
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

    int lastRow = firstRow + items.count(); //Last row + 1 extra to re-trigger possible next batch
    if(lastRow >= curItemCount)
        lastRow = curItemCount -1; //Ok, there is no extra row so notify just our batch

    if(firstRow > 0)
        firstRow--; //Try notify also the row before because there might be another batch waiting so re-trigger it
    QModelIndex firstIdx = index(firstRow, 0);
    QModelIndex lastIdx = index(lastRow, NCols - 1);
    emit dataChanged(firstIdx, lastIdx);
    emit itemsReady(firstRow, lastRow);

    qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}
