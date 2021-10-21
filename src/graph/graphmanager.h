#ifndef GRAPHMANAGER_H
#define GRAPHMANAGER_H

#include <QObject>

#include "utils/types.h"

typedef struct JobSelection
{
    db_id jobId;
    db_id segmentId;
} JobSelection;

class GraphManager : public QObject
{
    Q_OBJECT
public:
    explicit GraphManager(QObject *parent = nullptr);

    db_id getCurLineId() const;

    JobSelection getSelectedJob();
    void clearSelection();

signals:
    void jobSelected(db_id jobId);

private:
    db_id curLineId;
};

#endif // GRAPHMANAGER_H
