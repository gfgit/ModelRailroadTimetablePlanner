#ifndef SELECTSTATIONPAGE_H
#define SELECTSTATIONPAGE_H

#include <QWizardPage>

#include "utils/types.h"

class StationImportWizard;

class CustomCompletionLineEdit;
class ISqlFKMatchModel;

class QPushButton;

class SelectStationPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit SelectStationPage(StationImportWizard *w);

    void setupModel(ISqlFKMatchModel *m);
    void finalizeModel();

    void setStation(db_id stId, const QString& name);

private slots:
    void onCompletionDone();
    void openStationDlg();
    void openStationSVGPlan();
    void importSelectedStation();

private:
    StationImportWizard *mWizard;
    CustomCompletionLineEdit *stationLineEdit;
    QPushButton *openDlgBut;
    QPushButton *openSvgBut;
    QPushButton *importBut;

    ISqlFKMatchModel *stationsModel;

    QString mStName;
    db_id mStationId;
};

#endif // SELECTSTATIONPAGE_H
