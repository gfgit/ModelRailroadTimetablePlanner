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

#ifndef ROLLINGSTOCKMANAGER_H
#define ROLLINGSTOCKMANAGER_H

#include <QWidget>

#include "utils/types.h"

class QTableView;
class QToolBar;
class QTabWidget;

class RollingstockSQLModel;
class RSModelsSQLModel;
class RSOwnersSQLModel;

class SpinBoxEditorFactory;

class QActionGroup;

class RollingStockManager : public QWidget
{
    Q_OBJECT

public:
    enum Tabs
    {
        RollingstockTab = 0,
        ModelsTab,
        OwnersTab,
        NTabs
    };

    enum ModelState
    {
        ModelCleared = 0,
        ModelLoaded  = -1
    };

    enum
    {
        ClearModelTimeout = 5000
    }; // 5 seconds

    explicit RollingStockManager(QWidget *parent = nullptr);
    ~RollingStockManager();

    void setReadOnly(bool readOnly = true);

    static void importRS(bool resume, QWidget *parent);

private slots:
    void updateModels();
    void visibilityChanged(int v);

    void onModelError(const QString &msg);

    void onViewRSPlan();
    void onViewRSPlanSearch();

    void onNewRs();
    void onRemoveRs();
    void onRemoveAllRs();

    void onNewRsModel();
    void onNewRsModelWithDifferentSuffixFromCurrent();
    void onNewRsModelWithDifferentSuffixFromSearch();
    void onRemoveRsModel();
    void onRemoveAllRsModels();

    void onNewOwner();
    void onRemoveOwner();
    void onRemoveAllRsOwners();

    void onMergeModels();
    void onMergeOwners();

    void onImportRS();

    void showSessionRSViewer();

    void onRollingstockSelectionChanged();
    void onRsModelSelectionChanged();
    void onRsOwnerSelectionChanged();

protected:
    virtual void timerEvent(QTimerEvent *e) override;
    virtual void showEvent(QShowEvent *e) override;

private:
    void setupPages();
    bool createRsModelWithDifferentSuffix(db_id sourceModelId, QString &errMsg, QWidget *w);

private:
    QTabWidget *tabWidget;

    QToolBar *rsToolBar;
    QToolBar *modelToolBar;
    QToolBar *ownersToolBar;

    QActionGroup *editActGroup;

    QAction *actNewRs;
    QAction *actDeleteRs;
    QAction *actDeleteAllRs;

    QAction *actNewModel;
    QAction *actNewModelWithSuffix;
    QAction *actNewModelWithSuffixSearch;
    QAction *actDeleteModel;
    QAction *actDeleteAllRsModels;

    QAction *actNewOwner;
    QAction *actDeleteOwner;
    QAction *actDeleteAllRsOwners;

    QAction *actMergeModels;
    QAction *actMergeOwners;

    QAction *actViewRSPlan;
    QAction *actViewRSPlanSearch;

    QTableView *rsView;
    QTableView *rsModelsView;
    QTableView *ownersView;

    SpinBoxEditorFactory *speedSpinFactory;
    SpinBoxEditorFactory *axesSpinFactory;

    RollingstockSQLModel *rsSQLModel;
    RSModelsSQLModel *modelsSQLModel;
    RSOwnersSQLModel *ownersSQLModel;

    int oldCurrentTab;
    int clearModelTimers[NTabs];

    bool m_readOnly;
    bool windowConnected;
};

#endif // ROLLINGSTOCKMANAGER_H
