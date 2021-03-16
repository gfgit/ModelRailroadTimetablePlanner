#ifndef GRAPHMANAGER_H
#define GRAPHMANAGER_H

#include <QObject>

#include "utils/types.h"

class BackgroundHelper;
class GraphicsView;
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

    GraphicsView *getView() const;

    db_id getCurLineId() const;

    BackgroundHelper *getBackGround() const;

    JobSelection getSelectedJob();
    void clearSelection();

    db_id getFirstLineId();

signals:
    void currentLineChanged(db_id lineId);

    void jobSelected(db_id jobId);

public slots:
    bool setCurrentLine(db_id lineId);
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
    BackgroundHelper *backGround;
    GraphicsView *view;

    db_id curLineId;

    db_id curJobId;
};

#endif // GRAPHMANAGER_H
