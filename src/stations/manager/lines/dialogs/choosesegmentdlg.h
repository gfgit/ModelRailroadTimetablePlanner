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

    explicit ChooseSegmentDlg(sqlite3pp::database &db,
                              QWidget *parent = nullptr);

    virtual void done(int res) override;

    void setFilter(db_id fromStationId, db_id exceptSegment);

    bool getData(db_id& outSegId, QString& segName, bool &outIsReversed);

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
