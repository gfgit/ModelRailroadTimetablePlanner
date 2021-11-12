#ifndef GRAPHMANAGER_H
#define GRAPHMANAGER_H

#include <QObject>

#include "utils/types.h"

class LineStorage;

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
    ~GraphManager();

    db_id getCurLineId() const;

    JobSelection getSelectedJob();
    void clearSelection();

    db_id getFirstLineId();

signals:
    void currentLineChanged(db_id lineId);

    void jobSelected(db_id jobId);

public slots:
    void onSelectionChanged();
    void onSelectionCleared();

private slots:
    void onLineNameChanged(db_id lineId);
    void onLineModified(db_id lineId);
    void onLineRemoved(db_id lineId);

    void updateGraphOptions();

public:
    LineStorage *lineStorage;

private:

    db_id curLineId;

    db_id curJobId;
};

#endif // GRAPHMANAGER_H
