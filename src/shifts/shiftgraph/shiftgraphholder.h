#ifndef SHIFTGRAPHHOLDER_H
#define SHIFTGRAPHHOLDER_H

#include <QObject>
#include <QVector>
#include <QHash>

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>

using namespace sqlite3pp;

class QGraphicsScene;
class ShiftScene;

class ShiftGraphObj;

constexpr qreal MSEC_PER_HOUR = 1000 * 60 * 60;

//FIXME: make incremental load
class ShiftGraphHolder : public QObject
{
    Q_OBJECT
public:
    ShiftGraphHolder(database& db, QObject *parent = nullptr);
    ~ShiftGraphHolder();

    ShiftScene *scene() const;

    void loadShifts();

    bool getHideSameStations() const;
    void setHideSameStations(bool value);


    QString getShiftName(db_id shiftId) const;

public slots:
    void shiftNameChanged(db_id shiftId);
    void shiftRemoved(db_id shiftId);

    void shiftJobsChanged(db_id shiftId);

    void stationNameChanged(db_id stId);

    void redrawGraph();

    void updateJobColors();
    void updateShiftGraphOptions();

private:
    void drawShift(ShiftGraphObj *obj);

    void setObjPos(ShiftGraphObj *o, int pos);


    inline qreal jobPos(const QTime& t)
    {
        return t.msecsSinceStartOfDay() / MSEC_PER_HOUR * hourOffset + horizOffset;
    }

private:
    ShiftScene *mScene;

    QHash<db_id, ShiftGraphObj *> lookUp;
    QVector<ShiftGraphObj *> m_shifts;

    database& mDb;

    query q_selectShiftJobs; //FIXME: on demand

    qreal hourOffset = 150;
    qreal jobOffset = 50;

    qreal horizOffset = 50;
    qreal vertOffset = 20;

    qreal jobBoxOffset = 20;
    qreal stationNameOffset = 5;

    bool hideSameStations;
};

#endif // SHIFTGRAPHHOLDER_H
