#ifndef STATIONSMANAGER_H
#define STATIONSMANAGER_H

#include <QWidget>
#include <QDialog>

#include "utils/types.h"


class QToolBar;
class QToolButton;
class QTableView;

class StationsModel;
class RailwaySegmentsModel;
class LinesModel;

namespace Ui {
class StationsManager;
}

class StationsManager : public QWidget
{
    Q_OBJECT

public:

    typedef enum
    {
        StationsTab = 0,
        RailwaySegmentsTab,
        LinesTab,
        NTabs
    } Tabs;

    typedef enum
    {
        ModelCleared = 0,
        ModelLoaded = -1
    } ModelState;

    enum { ClearModelTimeout = 5000 }; // 5 seconds

    explicit StationsManager(QWidget *parent = nullptr);
    ~StationsManager() override;

    void fillLinesCombo();

    void setReadOnly(bool readOnly = true);

private slots:
    void updateModels();
    void visibilityChanged(int v);

    void onModelError(const QString &msg);

    void onRemoveStation();
    void onNewStation();
    void onEditStation();

    void showStJobViewer();
    void showStSVGPlan();
    void onShowFreeRS();

    void onRemoveSegment();
    void onNewSegment();
    void onEditSegment();

    void onNewLine();
    void onRemoveLine();
    void onEditLine();

    void onStationSelectionChanged();
    void onSegmentSelectionChanged();
    void onLineSelectionChanged();

    void onImportStations();

private:
    void setup_StationPage();
    void setup_SegmentPage();
    void setup_LinePage();

protected:
    virtual void showEvent(QShowEvent *e) override;
    virtual void timerEvent(QTimerEvent *e) override;

private:
    Ui::StationsManager *ui;

    QToolBar *stationToolBar;
    QToolBar *segmentsToolBar;
    QToolBar *linesToolBar;

    QTableView *stationView;
    QTableView *segmentsView;
    QTableView *linesView;

    StationsModel *stationsModel;
    RailwaySegmentsModel *segmentsModel;
    LinesModel *linesModel;

    QAction *act_addSt;
    QAction *act_remSt;
    QAction *act_editSt;
    QAction *act_stJobs;
    QAction *act_stSVG;
    QAction *act_freeRs;

    QAction *act_remSeg;
    QAction *act_editSeg;

    QAction *act_remLine;
    QAction *act_editLine;

    int oldCurrentTab;
    int clearModelTimers[NTabs];

    bool m_readOnly;
    bool windowConnected;
};

#endif // STATIONSMANAGER_H
