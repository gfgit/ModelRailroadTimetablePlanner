#include "stopcouplingmodel.h"

#include <QCoreApplication>

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "rslistondemandmodelresultevent.h"
#include "utils/rs_utils.h"


StopCouplingModel::StopCouplingModel(sqlite3pp::database &db, QObject *parent) :
    RSListOnDemandModel(db, parent),
    m_stopId(0),
    m_operation(RsOp::Uncoupled)
{
}

qint64 StopCouplingModel::recalcTotalItemCount()
{
    query q(mDb, "SELECT COUNT(1) FROM coupling WHERE stop_id=? AND operation=?");
    q.bind(1, m_stopId);
    q.bind(2, int(m_operation));
    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

void StopCouplingModel::setStop(db_id stopId, RsOp op)
{
    m_stopId = stopId;
    m_operation = op;

    refreshData(true);
}

void StopCouplingModel::internalFetch(int first, int sortCol, int valRow, const QVariant& val)
{
    query q(mDb);

    int offset = first - valRow + curPage * ItemsPerPage;
    bool reverse = false;

    if(valRow > first)
    {
        offset = 0;
        reverse = true;
    }

    //const char *whereCol;

    QByteArray sql = "SELECT coupling.rs_id,rs_list.number,rs_models.name,rs_models.suffix,rs_models.type"
                     " FROM coupling"
                     " JOIN rs_list ON rs_list.id=coupling.rs_id"
                     " LEFT JOIN rs_models ON rs_models.id=rs_list.model_id"
                     " WHERE coupling.stop_id=?2 AND coupling.operation=?3"
                     " ORDER BY rs_models.type,rs_models.name,rs_list.number,rs_models.suffix";
    //    switch (sortCol)
    //    {
    //    case Name:
    //    {
    //        whereCol = "name"; //Order by 2 columns, no where clause
    //        break;
    //    }
    //    }

    //    if(val.isValid())
    //    {
    //        sql += " WHERE ";
    //        sql += whereCol;
    //        if(reverse)
    //            sql += "<?3";
    //        else
    //            sql += ">?3";
    //    }

    //    sql += " ORDER BY ";
    //    sql += whereCol;

    //    if(reverse)
    //        sql += " DESC";

    sql += " LIMIT ?1";
    if(offset)
        sql += " OFFSET ?2";

    q.prepare(sql);
    q.bind(1, BatchSize);
    q.bind(2, m_stopId);
    q.bind(3, int(m_operation));
    if(offset)
        q.bind(2, offset);

    if(val.isValid())
    {
        switch (sortCol)
        {
        case Name:
        {
            q.bind(3, val.toString());
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
            item.rsId = r.get<db_id>(0);

            int number = r.get<int>(1);
            int modelNameLen = sqlite3_column_bytes(q.stmt(), 2);
            const char *modelName = reinterpret_cast<char const*>(sqlite3_column_text(q.stmt(), 2));

            int modelSuffixLen = sqlite3_column_bytes(q.stmt(), 3);
            const char *modelSuffix = reinterpret_cast<char const*>(sqlite3_column_text(q.stmt(), 3));
            item.type = RsType(sqlite3_column_int(q.stmt(), 4));

            item.name = rs_utils::formatNameRef(modelName, modelNameLen,
                                                number,
                                                modelSuffix, modelSuffixLen,
                                                item.type);
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
            item.rsId = r.get<db_id>(0);

            int number = r.get<int>(1);
            int modelNameLen = sqlite3_column_bytes(q.stmt(), 2);
            const char *modelName = reinterpret_cast<char const*>(sqlite3_column_text(q.stmt(), 2));

            int modelSuffixLen = sqlite3_column_bytes(q.stmt(), 3);
            const char *modelSuffix = reinterpret_cast<char const*>(sqlite3_column_text(q.stmt(), 3));
            item.type = RsType(sqlite3_column_int(q.stmt(), 4));

            item.name = rs_utils::formatNameRef(modelName, modelNameLen,
                                                number,
                                                modelSuffix, modelSuffixLen,
                                                item.type);
            i++;
        }
        if(i < BatchSize)
            vec.remove(i, BatchSize - i);
    }


    RSListOnDemandModelResultEvent *ev = new RSListOnDemandModelResultEvent;
    ev->items = vec;
    ev->firstRow = first;

    qApp->postEvent(this, ev);
}
