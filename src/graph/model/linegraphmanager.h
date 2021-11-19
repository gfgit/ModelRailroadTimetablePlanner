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

    /*!
     * \brief get active scene
     * \return Scene instance or nullptr if no scene is active
     */
    inline LineGraphScene *getActiveScene() const { return activeScene; }

signals:
    /*!
     * \brief Active scene is changed
     */
    void activeSceneChanged(LineGraphScene *scene);

public slots:
    /*!
     * \brief sets active scene
     *
     * This scene instance will be the active one and will therefore receive
     * user requests to show items
     *
     * \sa getActiveScene()
     * \sa activeSceneChanged()
     * \sa LineGraphScene::activateScene()
     */
    void setActiveScene(LineGraphScene *scene);

private slots:
    //Scenes
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

    //Jobs
    void onJobSelected(db_id jobId);

    //Settings
    void updateGraphOptions();

private:
    QVector<LineGraphScene *> scenes;
    LineGraphScene *activeScene;
};

#endif // LINEGRAPHMANAGER_H
