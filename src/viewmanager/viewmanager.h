#ifndef VIEWMANAGER_H
#define VIEWMANAGER_H

#include <QObject>

#include <QHash>

#include <QTime>

#include "utils/types.h"

class RollingStockManager;
class RSJobViewer;
class SessionStartEndRSViewer;

class StationsManager;
class StationJobView;
class StationFreeRSViewer;
class StationSVGPlanDlg;

class ShiftManager;
class ShiftViewer;
class ShiftGraphEditor;

class JobsManager;
class JobPathEditor;

class LineGraphManager;


class ViewManager : public QObject
{
    Q_OBJECT

    friend class MainWindow;
public:
    explicit ViewManager(QObject *parent = nullptr);

    bool closeEditors();
    void clearAllLineGraphs();

    inline LineGraphManager *getLineGraphMgr() const { return lineGraphManager; }

    bool requestJobSelection(db_id jobId, bool select, bool ensureVisible) const;
    bool requestJobEditor(db_id jobId, db_id stopId = 0);
    bool requestJobCreation();
    bool requestClearJob(bool evenIfEditing = false);
    bool removeSelectedJob();

    /*!
     * \brief requestJobShowPrevNextSegment
     * \param prev if true go to previous otherwise go to next
     * \param absolute if true go to first or last segment instead of prev/next
     * \return true on success
     *
     * Ensures prev/next segment of the job is visible.
     * If there is not segment before/after current in this graph then returns false.
     * Otherwise it will load a new graph and ensure prev/next segment is visible.
     */
    bool requestJobShowPrevNextSegment(bool prev, bool absolute);

    void requestRSInfo(db_id rsId);
    void requestStJobViewer(db_id stId);
    void requestStSVGPlan(db_id stId, bool showJobs = false, const QTime& time = QTime());
    void requestStFreeRSViewer(db_id stId);
    void requestShiftViewer(db_id id);
    void requestShiftGraphEditor();

    void showRSManager();
    void showStationsManager();
    void showShiftManager();
    void showJobsManager();

    void showSessionStartEndRSViewer();

    void resumeRSImportation();

private slots:

    void onRSRemoved(db_id rsId);
    void onRSPlanChanged(QSet<db_id> set);
    void onRSInfoChanged(db_id rsId);

    void onStRemoved(db_id stId);
    void onStNameChanged(db_id stId);
    void onStPlanChanged(const QSet<db_id> &stationIds);

    void onShiftRemoved(db_id shiftId);
    void onShiftEdited(db_id shiftId);
    void onShiftJobsChanged(db_id shiftId);

private:
    RSJobViewer* createRSViewer(db_id rsId);
    StationJobView *createStJobViewer(db_id stId);
    StationSVGPlanDlg *createStPlanDlg(db_id stId, QString &stNameOut);
    StationFreeRSViewer *createStFreeRSViewer(db_id stId);
    ShiftViewer *createShiftViewer(db_id id);

private:
    LineGraphManager *lineGraphManager;

    QWidget *m_mainWidget;

    RollingStockManager *rsManager;
    QHash<db_id, RSJobViewer *> rsHash;
    SessionStartEndRSViewer *sessionRSViewer;

    StationsManager *stManager;
    QHash<db_id, StationJobView *> stHash;
    QHash<db_id, StationFreeRSViewer *> stRSHash;
    QHash<db_id, StationSVGPlanDlg *> stPlanHash;

    ShiftManager *shiftManager;
    QHash<db_id, ShiftViewer *> shiftHash;
    ShiftGraphEditor *shiftGraphEditor;

    JobsManager *jobsManager;
    JobPathEditor *jobEditor;
};

#endif // VIEWMANAGER_H
