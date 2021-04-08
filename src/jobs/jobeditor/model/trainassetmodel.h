#ifndef TRAINASSETMODEL_H
#define TRAINASSETMODEL_H

#include "rslistondemandmodel.h"
#include <QTime>

class TrainAssetModel : public RSListOnDemandModel
{
    Q_OBJECT
public:
    typedef enum {
        BeforeStop,
        AfterStop
    } Mode;

    TrainAssetModel(sqlite3pp::database& db, QObject *parent = nullptr);

    // IPagedItemModel
    // Cached rows management
    virtual void refreshData(bool forceUpdate = false) override;

    // TrainAssetModel
    void setStop(db_id jobId, QTime arrival, Mode mode);

private:
    virtual void internalFetch(int first, int sortCol, int valRow, const QVariant &val) override;

private:
    db_id m_jobId;
    QTime m_arrival;
    Mode m_mode;
};

#endif // TRAINASSETMODEL_H
