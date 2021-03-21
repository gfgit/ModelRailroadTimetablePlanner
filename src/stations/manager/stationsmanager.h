#ifndef STATIONSMANAGER_H
#define STATIONSMANAGER_H

#include <QWidget>
#include <QDialog>

#include "utils/types.h"


class QToolBar;
class QToolButton;
class QTableView;

class StationsModel;
class LinesSQLModel;

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

    void onNewLine();
    void onRemoveLine();
    void onEditLine();
    void showStPlan();
    void onShowFreeRS();

private:
    void setup_StPage();
    void setup_LinePage();

protected:
    virtual void showEvent(QShowEvent *e) override;
    virtual void timerEvent(QTimerEvent *e) override;

private:
    Ui::StationsManager *ui;

    QToolBar *stationToolBar;
    QToolBar *linesToolBar;

    QTableView *stationView;
    QTableView *linesView;

    StationsModel *stationsModel;
    LinesSQLModel *linesModel;

    QAction *act_addSt;
    QAction *act_remSt;
    QAction *act_editSt;
    QAction *act_planSt;
    QAction *act_freeRs;

    int oldCurrentTab;
    int clearModelTimers[NTabs];

    bool m_readOnly;
    bool windowConnected;
};

#endif // STATIONSMANAGER_H
