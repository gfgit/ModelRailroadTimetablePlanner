#ifndef RS_ERROR_DATA_H
#define RS_ERROR_DATA_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include "utils/types.h"

#include <QTime>
#include <QVector>
#include <QMap>

namespace RsErrors
{

enum ErrType : quint32
{
    NoError = 0,
    StopIsTransit,
    CoupledWhileBusy, //otherId: previous coupling id
    UncoupledWhenNotCoupled, //otherId: previous uncoupling id
    NotUncoupledAtJobEnd, //otherId: next operation id or zero
    CoupledInDifferentStation, //otherId: previous coupling id
    UncoupledInSameStop //otherId: previous coupling id
};

struct RSErrorData
{
    db_id couplingId;
    db_id rsId;
    db_id stopId;
    db_id stationId;
    db_id otherId;
    JobEntry job;
    QString stationName;
    QTime time;
    ErrType errorType;
};

struct RSErrorList
{
    db_id rsId;
    QString rsName;
    QVector<RSErrorData> errors;

    inline int childCount() const { return errors.size(); }
    inline const RSErrorData *ptrForRow(int row) const { return &errors.at(row); }
};

struct RSErrorMap
{
    QMap<db_id, RSErrorList> map;

    inline int topLevelCount() const { return map.size(); }

    inline const RSErrorList *getTopLevelAtRow(int row) const
    {
        if(row >= topLevelCount())
            return nullptr;
        return &(map.constBegin() + row).value();
    }

    inline const RSErrorList *getParent(RSErrorData *child) const
    {
        auto it = map.constFind(child->rsId);
        if(it == map.constEnd())
            return nullptr;
        return &it.value();
    }

    inline int getParentRow(RSErrorData *child) const
    {
        auto it = map.constFind(child->rsId);
        if(it == map.constEnd())
            return -1;
        return std::distance(map.constBegin(), it);
    }
};

} //namespace RsErrors

#endif // ENABLE_BACKGROUND_MANAGER

#endif // RS_ERROR_DATA_H
