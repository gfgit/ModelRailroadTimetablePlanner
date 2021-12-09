#ifndef ERROR_DATA_H
#define ERROR_DATA_H

#ifdef ENABLE_RS_CHECKER

#include "utils/types.h"

#include <QTime>
#include <QVector>

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

typedef struct ErrorData_
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
} ErrorData;

typedef struct RSErrorList_
{
    db_id rsId;
    QString rsName;
    QVector<ErrorData> errors;
} RSErrorList;

} //namespace RsErrors

#endif // ENABLE_RS_CHECKER

#endif // ERROR_DATA_H
