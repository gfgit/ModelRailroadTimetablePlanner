#ifndef LINEGRAPHMANAGER_H
#define LINEGRAPHMANAGER_H

#include <QObject>

#include <QVector>

#include "utils/types.h"

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

private slots:
    void onSceneDestroyed(QObject *obj);

    void onStationNameChanged(db_id stationId);
    void onStationPlanChanged(db_id stationId);
    void onSegmentNameChanged(db_id segmentId);
    void onSegmentStationsChanged(db_id segmentId);
    void onLineNameChanged(db_id lineId);
    void onLineSegmentsChanged(db_id lineId);

    void updateGraphOptions();

private:
    QVector<LineGraphScene *> scenes;
};

#endif // LINEGRAPHMANAGER_H
