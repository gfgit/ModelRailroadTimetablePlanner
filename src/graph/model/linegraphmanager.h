#ifndef LINEGRAPHMANAGER_H
#define LINEGRAPHMANAGER_H

#include <QObject>

#include <QVector>

#include "utils/types.h"

class LineGraphScene;

class LineGraphManager : public QObject
{
    Q_OBJECT
public:
    explicit LineGraphManager(QObject *parent = nullptr);

    void registerScene(LineGraphScene *scene);
    void unregisterScene(LineGraphScene *scene);

    void clearAllGraphs();

private slots:
    void onSceneDestroyed(QObject *obj);

    void onStationNameChanged(db_id stationId);
    void onStationPlanChanged(db_id stationId);
    void onSegmentNameChanged(db_id segmentId);
    void onSegmentStationsChanged(db_id segmentId);
    void onLineNameChanged(db_id lineId);
    void onLineSegmentsChanged(db_id lineId);

private:
    QVector<LineGraphScene *> scenes;
};

#endif // LINEGRAPHMANAGER_H
