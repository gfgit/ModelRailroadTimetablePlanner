#ifndef RSPROXYMODEL_H
#define RSPROXYMODEL_H

#include <QAbstractListModel>

#include <QTime>

#include "utils/types.h"

class RSCouplingInterface;

class RSProxyModel : public QAbstractListModel
{
    Q_OBJECT

public:

    RSProxyModel(RSCouplingInterface *mgr,
                 RsOp o,
                 RsType type,
                 QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &idx, const QVariant &value, int role) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    enum RSItemFlg
    {
        NoFlag = 0,
        WrongStation = 1,
        LastOperation = 2,
        HasNextOperation = 3,
        FirstUseOfRS = 4,
        UnusedRS = 5,
        ErrNotCoupledBefore = 6,
        ErrAlreadyCoupled = 7
    };

    struct RsItem
    {
        db_id rsId;
        db_id jobId; //Can be next job or previous job
        QString rsName;
        int flag;
        QTime time; //Can be next or previous operation time
        JobCategory jobCat;
        RsEngineSubType engineType;
    };

    void loadData(const QVector<RsItem>& items);

private:
    QVector<RsItem> m_data;

    RSCouplingInterface *couplingMgr;
    RsOp op;
    RsType targetType;
};

#endif // RSPROXYMODEL_H
