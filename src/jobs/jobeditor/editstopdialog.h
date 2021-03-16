#ifndef EDITSTOPDIALOG2_H
#define EDITSTOPDIALOG2_H

#include <QDialog>
#include <QTime>
#include <QSet>
#include <QPersistentModelIndex>

#include "utils/types.h"

class CustomCompletionLineEdit;
class StopModel;
class TrainAssetModel;
class RSCouplingInterface;
class JobPassingsModel;
class StopCouplingModel;
class StationsMatchModel;

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

    void updateSpeedAfterStop();

    bool hasEngineAfterStop();

    QSet<db_id> getRsToUpdate() const;

    void setStop(StopModel *stops, const QModelIndex &idx);

    void clearUi();

    void setReadOnly(bool value);

public slots:
    void done(int val) override;

    void editCoupled();
    void editUncoupled();

    void calcTime();
    void applyTime();

    void onPlatfRadioToggled(bool enable);

    void calcPassings();

    void couplingCustomContextMenuRequested(const QPoint &pos);

private:
    void saveDataToModel();
    void updateDistance();

    void showBeforeAsset(bool val);
    void showAfterAsset(bool val);

    void setPlatformCount(int maxMainPlatf, int maxDepots);
    void setPlatform(int platf);
    int getPlatform();

    int getTrainSpeedKmH(bool afterStop);

private:
    Ui::EditStopDialog *ui;
    CustomCompletionLineEdit *stationLineEdit;
    StationsMatchModel *stationsMatchModel;

    db_id m_jobId;
    db_id m_stopId;

    db_id m_stationId;
    db_id m_prevStId;

    db_id curSegment;
    db_id curLine;

    StopModel *stopModel;
    QPersistentModelIndex stopIdx;

    StopType stopType;
    JobCategory m_jobCat;

    QTime previousDep;

    QTime originalArrival;
    QTime originalDeparture;

    int speedBeforeStop;
    int originalSpeedAfterStopKmH;
    int newSpeedAfterStopKmH;

    QSet<db_id> rsToUpdate;

    RSCouplingInterface *couplingMgr;

    StopCouplingModel *coupledModel;
    StopCouplingModel *uncoupledModel;

    TrainAssetModel    *trainAssetModelBefore;
    TrainAssetModel    *trainAssetModelAfter;

    JobPassingsModel *passingsModel;
    JobPassingsModel *crossingsModel;

    bool readOnly;
};

#endif // EDITSTOPDIALOG2_H
