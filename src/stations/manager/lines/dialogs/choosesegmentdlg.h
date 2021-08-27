#ifndef CHOOSESEGMENTDLG_H
#define CHOOSESEGMENTDLG_H

#include <QDialog>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class CustomCompletionLineEdit;

class StationsMatchModel;
class RailwaySegmentMatchModel;

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

    void setFilter(db_id fromStationId, db_id toStationId, db_id exceptSegment);

    bool getData(db_id& outSegId, QString& segName, bool &outIsReversed);

private slots:
    void onStationChanged();
    void onSegmentSelected(db_id segmentId);

private:
    CustomCompletionLineEdit *fromStationEdit;
    CustomCompletionLineEdit *toStationEdit;
    CustomCompletionLineEdit *segmentEdit;

    StationsMatchModel *fromStationMatch;
    StationsMatchModel *toStationMatch;
    RailwaySegmentMatchModel *segmentMatch;

    db_id lockFromStationId;
    db_id lockToStationId;
    db_id excludeSegmentId;
    bool isReversed;
};

#endif // CHOOSESEGMENTDLG_H
