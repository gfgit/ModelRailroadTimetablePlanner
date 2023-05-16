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

    enum Tabs
    {
        StationsTab = 0,
        RailwaySegmentsTab,
        LinesTab,
        NTabs
    };

    enum ModelState
    {
        ModelCleared = 0,
        ModelLoaded = -1
    };

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
    void onSplitSegment();

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
