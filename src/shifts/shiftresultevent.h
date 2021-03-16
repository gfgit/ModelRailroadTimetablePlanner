#ifndef SHIFTRESULTEVENT_H
#define SHIFTRESULTEVENT_H

#include <QEvent>
#include "utils/worker_event_types.h"

#include <QVector>
#include "shiftitem.h"

class ShiftResultEvent : public QEvent
{
public:
    static const Type _Type = Type(CustomEvents::ShiftWorkerResult);

    ShiftResultEvent();

    QVector<ShiftItem> items;
    int firstRow;
};

#endif // SHIFTRESULTEVENT_H
