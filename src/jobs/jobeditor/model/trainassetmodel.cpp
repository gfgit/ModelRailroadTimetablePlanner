#include "trainassetmodel.h"

#include "utils/delegates/sql/pageditemmodelhelper_impl.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/rs_utils.h"

#include <QDebug>

TrainAssetModel::TrainAssetModel(database& db, QObject *parent) :
    RSListOnDemandModel(db, parent),
    m_jobId(0),
    m_mode(BeforeStop)
{
}

qint64 TrainAssetModel::recalcTotalItemCount()
{
    query q(mDb, "SELECT COUNT(1) FROM("
                 "SELECT coupling.rs_id,MAX(stops.arrival)"
                 " FROM stops"
                 " JOIN coupling ON coupling.stop_id=stops.id"
                 " WHERE stops.job_id=? AND stops.arrival<?"
                 " GROUP BY coupling.rs_id"
                 " HAVING coupling.operation=1)");
    q.bind(1, m_jobId);
    //HACK: 1 minute is the min interval between stops,
    //by adding 1 minute we include the current stop but leave out the next one
    if(m_mode == AfterStop)
        q.bind(2, m_arrival.addSecs(60));
    else
        q.bind(2, m_arrival);
    int ret = q.step();
    if(ret != SQLITE_ROW)
        qWarning() << "TrainAssetModel: " << mDb.error_msg() << mDb.error_code();

    const qint64 count = q.getRows().get<int>(0);
    return count;
}

void TrainAssetModel::internalFetch(int first, int sortCol, int /*valRow*/, const QVariant &/*val*/)
{
    query q(mDb);

    int offset = first + curPage * ItemsPerPage;

    //const char *whereCol;

    QByteArray sql = "SELECT sub.rs_id,sub.number,sub.name,sub.suffix,sub.type FROM("
                     "SELECT coupling.rs_id,rs_list.number,rs_models.name,rs_models.suffix,rs_models.type,MAX(stops.arrival)"
                     " FROM stops"
                     " JOIN coupling ON coupling.stop_id=stops.id"
                     " JOIN rs_list ON rs_list.id=rs_id"
                     " LEFT JOIN rs_models ON rs_models.id=rs_list.model_id"
                     " WHERE stops.job_id=?3 AND stops.arrival<?4"
                     " GROUP BY coupling.rs_id"
                     " HAVING coupling.operation=1) AS sub"
                     " ORDER BY sub.type,sub.name,sub.number,sub.suffix";
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
    q.bind(3, m_jobId);
    //HACK: 1 minute is the min interval between stops,
    //by adding 1 minute we include the current stop but leave out the next one
    if(m_mode == AfterStop)
        q.bind(4, m_arrival.addSecs(60));
    else
        q.bind(4, m_arrival);
    if(offset)
        q.bind(2, offset);

    QVector<RSItem> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

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

    postResult(vec, first);
}

void TrainAssetModel::setStop(db_id jobId, QTime arrival, Mode mode)
{
    m_jobId = jobId;
    m_arrival = arrival;
    m_mode = mode;

    refreshData(true);
}
