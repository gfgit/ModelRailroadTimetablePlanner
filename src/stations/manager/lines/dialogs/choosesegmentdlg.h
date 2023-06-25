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

#ifndef CHOOSESEGMENTDLG_H
#define CHOOSESEGMENTDLG_H

#include <QDialog>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class CustomCompletionLineEdit;

class StationsMatchModel;
class StationGatesMatchModel;

class ChooseSegmentDlg : public QDialog
{
    Q_OBJECT
public:
    enum LockField
    {
        DoNotLock = 0
    };

    explicit ChooseSegmentDlg(sqlite3pp::database &db, QWidget *parent = nullptr);

    virtual void done(int res) override;

    void setFilter(db_id fromStationId, db_id exceptSegment);

    bool getData(db_id &outSegId, QString &segName, bool &outIsReversed);

private slots:
    void onStationChanged();
    void onSegmentSelected(const QModelIndex &idx);

private:
    CustomCompletionLineEdit *fromStationEdit;
    CustomCompletionLineEdit *outGateEdit;

    StationsMatchModel *fromStationMatch;
    StationGatesMatchModel *gateMatch;

    db_id lockFromStationId;
    db_id excludeSegmentId;
    db_id selectedSegmentId;
    bool isReversed;
};

#endif // CHOOSESEGMENTDLG_H
