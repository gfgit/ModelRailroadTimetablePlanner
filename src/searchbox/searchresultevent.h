#ifndef SEARCHRESULTEVENT_H
#define SEARCHRESULTEVENT_H

#ifdef SEARCHBOX_MODE_ASYNC

#include <QEvent>

#include <QVector>

#include "utils/worker_event_types.h"

class SearchTask;
struct SearchResultItem;

class SearchResultEvent : public QEvent
{
public:
    static const Type _Type = Type(CustomEvents::SearchBoxResults);

    SearchResultEvent(SearchTask *ta, const QVector<SearchResultItem> &vec);
    virtual ~SearchResultEvent();

public:
    SearchTask *task;
    QVector<SearchResultItem> results;
};

#endif //SEARCHBOX_MODE_ASYNC

#endif // SEARCHRESULTEVENT_H
