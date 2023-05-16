/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LINEGRAPHMANAGER_H
#define LINEGRAPHMANAGER_H

#include <QObject>

#include <QVector>

#include "utils/types.h"
#include "graph/linegraphtypes.h"

class IGraphScene;
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
     * \brief react to line graph update events
     *
     * \sa processPendingUpdates()
     */
    bool event(QEvent *ev) override;

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

    /*!
     * \brief schedule async update
     *
     * If an update is already schedule, does nothing.
     * Otherwise posts an event to self so it will update
     * when Qt event loop is not busy
     *
     * \sa processPendingUpdates()
     */
    void scheduleUpdate();

    /*!
     * \brief process pending updates
     *
     * Updates all scenes marked for update
     *
     * \sa scheduleUpdate()
     * \sa event()
     */
    void processPendingUpdates();

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
    void setActiveScene(IGraphScene *scene);

private slots:
    //Scenes
    void onSceneDestroyed(QObject *obj);
    void onGraphChanged(int graphType_, db_id graphObjId, LineGraphScene *scene);
    void onJobSelected(db_id jobId, int category, db_id stopId);

    //Stations
    void onStationNameChanged(db_id stationId);
    void onStationJobPlanChanged(const QSet<db_id> &stationIds);
    void onStationTrackPlanChanged(const QSet<db_id> &stationIds);
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
    void onJobRemoved(db_id jobId);

    //Settings
    void updateGraphOptions();

private:
    void onStationPlanChanged_internal(const QSet<db_id> &stationIds, int flag);

private:
    QVector<LineGraphScene *> scenes;
    LineGraphScene *activeScene;
    JobStopEntry lastSelectedJob;
    bool m_followJobOnGraphChange;
    bool m_hasScheduledUpdate;
};

#endif // LINEGRAPHMANAGER_H
