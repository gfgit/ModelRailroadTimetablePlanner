#ifndef LINEGRAPHMANAGER_H
#define LINEGRAPHMANAGER_H

#include <QObject>

#include <QVector>

#include "utils/types.h"
#include "graph/linegraphtypes.h"

class LineGraphScene;

/*!
 * \brief Class for managing LineGraphScene instances
 *
 * The manager listens for changes on railway plan and
 * decides which of the registered LineGraphScene needs
 * to be updated.
 *
 * \sa LineGraphScene
 */
class LineGraphManager : public QObject
{
    Q_OBJECT
public:
    explicit LineGraphManager(QObject *parent = nullptr);

    void registerScene(LineGraphScene *scene);
    void unregisterScene(LineGraphScene *scene);

    void clearAllGraphs();
    void clearGraphsOfObject(db_id objectId, LineGraphType type);

private slots:
    void onSceneDestroyed(QObject *obj);

    //Stations
    void onStationNameChanged(db_id stationId);
    void onStationPlanChanged(db_id stationId);
    void onStationRemoved(db_id stationId);

    //Segments
    void onSegmentNameChanged(db_id segmentId);
    void onSegmentStationsChanged(db_id segmentId);
    void onSegmentRemoved(db_id segmentId);

    //Lines
    void onLineNameChanged(db_id lineId);
    void onLineSegmentsChanged(db_id lineId);
    void onLineRemoved(db_id lineId);

    void updateGraphOptions();

private:
    QVector<LineGraphScene *> scenes;
};

#endif // LINEGRAPHMANAGER_H
