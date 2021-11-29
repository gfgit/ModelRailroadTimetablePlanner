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

    /*!
     * \brief subscribe scene to notifications
     *
     * The scene gets registered on this manager and will be refreshed
     * we railway layout changes.
     * The first scene registered is set as active
     *
     * \sa unregisterScene()
     * \sa setActiveScene()
     */
    void registerScene(LineGraphScene *scene);

    /*!
     * \brief unsubscribe scene from notifications
     *
     * The scene will not be refreshed by this manager anymore
     * If it was the active scene, active scene will be reset
     *
     * \sa registerScene()
     * \sa setActiveScene()
     */
    void unregisterScene(LineGraphScene *scene);

    void clearAllGraphs();
    void clearGraphsOfObject(db_id objectId, LineGraphType type);

    /*!
     * \brief get active scene
     * \return Scene instance or nullptr if no scene is active
     */
    inline LineGraphScene *getActiveScene() const { return activeScene; }

    /*!
     * \brief get current selected job
     *
     * If there is an active scene, return it's selection otherwise null JobStopEntry::jobId
     * \sa getActiveScene()
     * \sa LineGraphScene::getSelectedJob()
     */
    JobStopEntry getCurrentSelectedJob() const;

signals:
    /*!
     * \brief Active scene is changed
     */
    void activeSceneChanged(LineGraphScene *scene);

    /*!
     * \brief jobSelected
     * \param jobId
     * \param category
     * \param stopId
     *
     * Emitted when \a active scene job selection changes or if active scene changes.
     * Not emitted for other scenes.
     *
     * \sa getActiveScene()
     * \sa getCurrentSelectedJob()
     */
    void jobSelected(db_id jobId, int category, db_id stopId);

public slots:
    /*!
     * \brief sets active scene
     *
     * This scene instance will be the active one and will therefore receive
     * user requests to show items.
     * Scene must be registered on this manager first.
     * If scene parameter is nullptr (i.e. when resetting active scene) we try
     * to activate our first registered scene.
     *
     * \sa getActiveScene()
     * \sa activeSceneChanged()
     * \sa LineGraphScene::activateScene()
     */
    void setActiveScene(LineGraphScene *scene);

private slots:
    //Scenes
    void onSceneDestroyed(QObject *obj);
    void onGraphChanged(int graphType_, db_id graphObjId, LineGraphScene *scene);
    void onJobSelected(db_id jobId, int category, db_id stopId);

    //Stations
    void onStationNameChanged(db_id stationId);
    void onStationPlanChanged(const QSet<db_id> &stationIds);
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
    void onJobChanged(db_id jobId, db_id oldJobId);

    //Settings
    void updateGraphOptions();

private:
    QVector<LineGraphScene *> scenes;
    LineGraphScene *activeScene;
    JobStopEntry lastSelectedJob;
    bool m_followJobOnGraphChange;
};

#endif // LINEGRAPHMANAGER_H
