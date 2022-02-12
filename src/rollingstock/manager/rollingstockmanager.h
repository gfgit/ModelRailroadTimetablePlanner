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
        ModelLoaded = -1
    };

    enum { ClearModelTimeout = 5000 }; // 5 seconds

    explicit RollingStockManager(QWidget *parent = nullptr);
    ~RollingStockManager();

    void setReadOnly(bool readOnly = true);

    static void importRS(bool resume, QWidget *parent);

private slots:
    void updateModels();
    void visibilityChanged(int v);

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
    QTabWidget        *tabWidget;

    QToolBar          *rsToolBar;
    QToolBar          *modelToolBar;
    QToolBar          *ownersToolBar;

    QActionGroup      *editActGroup;

    QAction           *actNewRs;
    QAction           *actDeleteRs;
    QAction           *actDeleteAllRs;

    QAction           *actNewModel;
    QAction           *actNewModelWithSuffix;
    QAction           *actNewModelWithSuffixSearch;
    QAction           *actDeleteModel;
    QAction           *actDeleteAllRsModels;

    QAction           *actNewOwner;
    QAction           *actDeleteOwner;
    QAction           *actDeleteAllRsOwners;

    QAction           *actMergeModels;
    QAction           *actMergeOwners;

    QAction           *actViewRSPlan;
    QAction           *actViewRSPlanSearch;

    QTableView        *rsView;
    QTableView        *rsModelsView;
    QTableView        *ownersView;

    SpinBoxEditorFactory *speedSpinFactory;
    SpinBoxEditorFactory *axesSpinFactory;

    RollingstockSQLModel *rsSQLModel;
    RSModelsSQLModel  *modelsSQLModel;
    RSOwnersSQLModel  *ownersSQLModel;


    int oldCurrentTab;
    int clearModelTimers[NTabs];

    bool m_readOnly;
    bool windowConnected;
};

#endif // ROLLINGSTOCKMANAGER_H
