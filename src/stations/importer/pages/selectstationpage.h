#ifndef SELECTSTATIONPAGE_H
#define SELECTSTATIONPAGE_H

#include <QWizardPage>

#include "utils/types.h"

class StationImportWizard;

class QToolBar;
class QAction;
class QLineEdit;
class QTableView;
class ImportStationModel;

class SelectStationPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit SelectStationPage(StationImportWizard *w);


    void setupModel(ImportStationModel *m);
    void finalizeModel();

private slots:
    void updateFilter();
    void openStationDlg();
    void openStationSVGPlan();
    void importSelectedStation();

    void startFilterTimer();

private:
    void stopFilterTimer();

protected:
    void timerEvent(QTimerEvent *e) override;

private:
    StationImportWizard *mWizard;

    QToolBar *toolBar;
    QAction *actOpenStDlg;
    QAction *actOpenSVGPlan;
    QAction *actImportSt;

    QLineEdit *filterNameEdit;
    QTableView *view;

    ImportStationModel *stationsModel;

    int filterTimerId;
};

#endif // SELECTSTATIONPAGE_H
