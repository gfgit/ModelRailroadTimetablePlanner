#ifndef STOPCOUPLINGMODEL_H
#define STOPCOUPLINGMODEL_H

#include "rslistondemandmodel.h"


class StopCouplingModel : public RSListOnDemandModel
{
    Q_OBJECT

public:
    StopCouplingModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // IPagedItemModel
    // Cached rows management
    virtual void refreshData() override;

    // StopCouplingModel
    void setStop(db_id stopId, RsOp op);

private:
    virtual void internalFetch(int first, int sortColumn, int valRow, const QVariant &val) override;

private:
    db_id m_stopId;
    RsOp m_operation;
};

#endif // STOPCOUPLINGMODEL_H
