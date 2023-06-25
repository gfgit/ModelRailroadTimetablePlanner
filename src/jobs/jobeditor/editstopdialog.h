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

#ifndef EDITSTOPDIALOG2_H
#define EDITSTOPDIALOG2_H

#include <QDialog>
#include <QPersistentModelIndex>

#include "utils/types.h"

class StopEditingHelper;

class StopModel;
struct StopItem;

class TrainAssetModel;
class RSCouplingInterface;
class JobPassingsModel;
class StopCouplingModel;

namespace Ui {
class EditStopDialog;
}

class EditStopDialog : public QDialog
{
    Q_OBJECT

public:
    EditStopDialog(StopModel *m, QWidget *parent = nullptr);
    ~EditStopDialog() override;

    void updateInfo();

    bool hasEngineAfterStop();

    void setStop(const QModelIndex &idx);

    void clearUi();

    void setReadOnly(bool value);

public slots:
    void done(int val) override;

private slots:
    void importJobRS();
    void editCoupled();
    void editUncoupled();

    void calcPassings();

    void couplingCustomContextMenuRequested(const QPoint &pos);

    void updateAdditionalNotes();

private:
    void saveDataToModel();

    void showBeforeAsset(bool val);
    void showAfterAsset(bool val);

    int getTrainSpeedKmH(bool afterStop);

private:
    Ui::EditStopDialog *ui;

    StopEditingHelper *helper;

    db_id m_jobId;
    JobCategory m_jobCat;

    StopModel *stopModel;
    QPersistentModelIndex stopIdx;

    RSCouplingInterface *couplingMgr;

    StopCouplingModel *coupledModel;
    StopCouplingModel *uncoupledModel;

    TrainAssetModel *trainAssetModelBefore;
    TrainAssetModel *trainAssetModelAfter;

    JobPassingsModel *passingsModel;
    JobPassingsModel *crossingsModel;

    bool readOnly;
};

#endif // EDITSTOPDIALOG2_H
