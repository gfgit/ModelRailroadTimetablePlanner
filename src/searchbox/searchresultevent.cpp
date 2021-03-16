#ifdef SEARCHBOX_MODE_ASYNC

#include "searchresultevent.h"

#include "searchresultitem.h"

SearchResultEvent::SearchResultEvent(SearchTask *ta, const QVector<SearchResultItem> &vec) :
    QEvent(_Type),
    task(ta),
    results(vec)
{

}

SearchResultEvent::~SearchResultEvent()
{

}

#endif //SEARCHBOX_MODE_ASYNC
