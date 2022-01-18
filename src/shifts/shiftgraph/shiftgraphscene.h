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

class QPainter;

constexpr qreal MSEC_PER_HOUR = 1000 * 60 * 60;

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

    void drawShifts(QPainter *painter, const QRectF& sceneRect);
    void drawShiftHeader(QPainter *painter, const QRectF& rect, double vertScroll);
    void drawHourHeader(QPainter *painter, const QRectF &rect, double horizScroll);

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

    inline qreal jobPos(const QTime& t)
    {
        return t.msecsSinceStartOfDay() / MSEC_PER_HOUR * hourOffset + horizOffset;
    }

private:
    sqlite3pp::database& mDb;

    QVector<ShiftRow> m_shifts;
    QHash<db_id, QString> m_stationCache;

    //Options
    qreal hourOffset = 150;
    qreal shiftOffset = 50;
    qreal spaceOffset = 10;

    qreal horizOffset = 50;
    qreal vertOffset = 20;

    bool hideSameStations = true;
};

#endif // SHIFTGRAPHSCENE_H
