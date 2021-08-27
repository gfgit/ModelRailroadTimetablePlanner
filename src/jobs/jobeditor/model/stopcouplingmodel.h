#ifndef STOPCOUPLINGMODEL_H
#define STOPCOUPLINGMODEL_H

#include "rslistondemandmodel.h"


class StopCouplingModel : public RSListOnDemandModel
{
    Q_OBJECT

public:
    StopCouplingModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // StopCouplingModel
    void setStop(db_id stopId, RsOp op);

protected:
    // IPagedItemModel
    // Cached rows management
    virtual qint64 recalcTotalItemCount() override;

private:
    virtual void internalFetch(int first, int sortColumn, int valRow, const QVariant &val) override;

private:
    db_id m_stopId;
    RsOp m_operation;
};

#endif // STOPCOUPLINGMODEL_H
