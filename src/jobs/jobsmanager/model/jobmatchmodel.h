#ifndef JOBMATCHMODEL_H
#define JOBMATCHMODEL_H

#include <QFont>

#include "utils/delegates/sql/isqlfkmatchmodel.h"

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

//TODO: share common code with SearchResultModel
//TODO: allow filter byy job category

/*!
 * \brief Match model for Jobs
 *
 * Support filtering out 1 job (i.e. get 'others')
 * or get only jobs which stop in a specific station.
 * When specifying a station, you can also set a maximum time for arrival.
 * All jobs arriving later are excluded.
 * Otherwhise the arrival parameter is ignored if no station is set.
 */
class JobMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT

public:
    enum Column
    {
        JobCategoryCol = 0,
        JobNumber,
        StopArrivalCol,
        NCols
    };

    enum DefaultId : bool
    {
        JobId = 0,
        StopId
    };

    JobMatchModel(database &db, QObject *parent = nullptr);

    // Basic functionality:
    int columnCount(const QModelIndex &p = QModelIndex()) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel:
    void autoSuggest(const QString& text) override;
    virtual void refreshData() override;
    QString getName(db_id jobId) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

    // JobMatchModel:
    void setFilter(db_id exceptJobId, db_id stopsInStationId, const QTime& maxStopArr);

    void setDefaultId(DefaultId defaultId);

private:
    struct JobItem
    {
        JobStopEntry stop;
        QTime stopArrival;
    };

    JobItem items[MaxMatchItems];

    database &mDb;
    query q_getMatches;

    db_id m_exceptJobId;
    db_id m_stopStationId;
    QTime m_maxStopArrival;
    DefaultId m_defaultId;

    QByteArray mQuery;

    QFont m_font;
};

#endif // JOBMATCHMODEL_H
