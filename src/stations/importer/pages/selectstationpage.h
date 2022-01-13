#ifndef SELECTSTATIONPAGE_H
#define SELECTSTATIONPAGE_H

#include <QWizardPage>

#include "utils/types.h"

class StationImportWizard;

class QToolBar;
class QAction;
class QTableView;
class ImportStationModel;
class ModelPageSwitcher;

class SelectStationPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit SelectStationPage(StationImportWizard *w);

    void setupModel(ImportStationModel *m);
    void finalizeModel();

private slots:
    void openStationDlg();
    void openStationSVGPlan();
    void importSelectedStation();

private:
    StationImportWizard *mWizard;

    QToolBar *toolBar;
    QAction *actOpenStDlg;
    QAction *actOpenSVGPlan;
    QAction *actImportSt;

    QTableView *view;
    ModelPageSwitcher *pageSwitcher;

    ImportStationModel *stationsModel;
};

#endif // SELECTSTATIONPAGE_H
