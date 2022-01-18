#ifndef SHIFTGRAPHSCENE_H
#define SHIFTGRAPHSCENE_H

#include <QObject>
#include <QVector>
#include <QHash>
#include <QTime>

#include "utils/types.h"

namespace sqlite3pp {
class database;
class query;
}

class ShiftGraphScene : public QObject
{
    Q_OBJECT
public:

    struct JobItem
    {
        JobEntry job;

        db_id fromStId;
        db_id toStId;

        QTime start;
        QTime end;
    };

    struct ShiftRow
    {
        db_id shiftId;
        QString shiftName;

        QVector<JobItem> jobList;
    };

    ShiftGraphScene(sqlite3pp::database& db, QObject *parent = nullptr);

public slots:
    bool loadShifts();

signals:
    void redrawGraph();

private slots:
    void updateShiftGraphOptions();

    void onShiftNameChanged(db_id shiftId);
    void onShiftRemoved(db_id shiftId);
    void onShiftJobsChanged(db_id shiftId);

    void onStationNameChanged(db_id stationId);

private:
    bool loadShiftRow(ShiftRow& shiftObj, sqlite3pp::query& q_getStName,
                      sqlite3pp::query& q_countJobs, sqlite3pp::query& q_getJobs);
    void loadStationName(db_id stationId, sqlite3pp::query& q_getStName);

    std::pair<int, int> lowerBound(db_id shiftId, const QString& name);

private:
    sqlite3pp::database& mDb;

    QVector<ShiftRow> m_shifts;
    QHash<db_id, QString> m_stationCache;

    //Options
    qreal hourOffset = 150;
    qreal jobOffset = 50;

    qreal horizOffset = 50;
    qreal vertOffset = 20;

    qreal jobBoxOffset = 20;
    qreal stationNameOffset = 5;

    bool hideSameStations = true;
};

#endif // SHIFTGRAPHSCENE_H
