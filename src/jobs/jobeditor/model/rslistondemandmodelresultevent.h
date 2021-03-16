#ifndef RSLISTONDEMANDMODELRESULTEVENT_H
#define RSLISTONDEMANDMODELRESULTEVENT_H

#include "rslistondemandmodel.h"
#include <QEvent>
#include "utils/worker_event_types.h"

class RSListOnDemandModelResultEvent : public QEvent
{
public:
    static constexpr Type _Type = Type(CustomEvents::RsOnDemandListModelResult);

    RSListOnDemandModelResultEvent();

    QVector<RSListOnDemandModel::RSItem> items;
    int firstRow;
};

#endif // RSLISTONDEMANDMODELRESULTEVENT_H
