#ifndef EDITSTOPDIALOG2_H
#define EDITSTOPDIALOG2_H

#include <QDialog>
#include <QTime>
#include <QSet>
#include <QPersistentModelIndex>

#include "utils/types.h"

#include "jobs/jobeditor/model/stopmodel.h" //TODO: include only StopItem

class StopModel;
class TrainAssetModel;
class RSCouplingInterface;
class JobPassingsModel;
class StopCouplingModel;

class CustomCompletionLineEdit;

class StationsMatchModel;
class StationGatesMatchModel;
class StationTracksMatchModel;

namespace Ui {
class EditStopDialog;
}

class EditStopDialog : public QDialog
{
    Q_OBJECT

public:
    EditStopDialog(QWidget *parent = nullptr);
    ~EditStopDialog() override;

    void updateInfo();
    void onStEditingFinished(db_id stationId);

    bool hasEngineAfterStop();

    QSet<db_id> getRsToUpdate() const;

    void setStop(StopModel *stops, const QModelIndex &idx);

    void clearUi();

    void setReadOnly(bool value);

public slots:
    void done(int val) override;

    void editCoupled();
    void editUncoupled();

    void calcPassings();

    void couplingCustomContextMenuRequested(const QPoint &pos);

private:
    void saveDataToModel();

    void showBeforeAsset(bool val);
    void showAfterAsset(bool val);

    void setPlatformCount(int maxMainPlatf, int maxDepots);

    int getTrainSpeedKmH(bool afterStop);

private:
    Ui::EditStopDialog *ui;

    CustomCompletionLineEdit *stationEdit;
    CustomCompletionLineEdit *stationTrackEdit;
    CustomCompletionLineEdit *outGateEdit;

    StationsMatchModel *stationMatchModel;
    StationGatesMatchModel *stationOutGateMatchModel;
    StationTracksMatchModel *stationTrackMatchModel;

    db_id m_jobId;
    JobCategory m_jobCat;

    StopModel *stopModel;
    QPersistentModelIndex stopIdx;

    QSet<db_id> rsToUpdate;

    RSCouplingInterface *couplingMgr;

    StopCouplingModel *coupledModel;
    StopCouplingModel *uncoupledModel;

    TrainAssetModel   *trainAssetModelBefore;
    TrainAssetModel   *trainAssetModelAfter;

    JobPassingsModel  *passingsModel;
    JobPassingsModel  *crossingsModel;

    StopItem curStop;
    StopItem prevStop;

    bool readOnly;
};

#endif // EDITSTOPDIALOG2_H
