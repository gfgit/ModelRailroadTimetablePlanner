#ifndef LINESTORAGE_H
#define LINESTORAGE_H

#include <QObject>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class LineStoragePrivate;
class QGraphicsScene;

class LineStorage : public QObject
{
    Q_OBJECT
public:
    LineStorage(sqlite3pp::database &db, QObject *parent = nullptr);
    ~LineStorage();

    void clear();

    bool addLine(db_id *outLineId = nullptr);
    bool removeLine(db_id lineId);

    bool addStation(db_id *outStationId = nullptr);
    bool removeStation(db_id stationId);

    void addStationToLine(db_id lineId, db_id stId);
    void removeStationFromLine(db_id lineId, db_id stId);

    bool releaseLine(db_id lineId);
    bool increfLine(db_id lineId);
    QGraphicsScene *sceneForLine(db_id lineId);

    void redrawAllLines();
    void redrawLine(db_id lineId);

    void addJobStop(db_id stopId, db_id stId, db_id jobId, db_id lineId,
                    const QString &label, qreal y1, qreal y2, int platf);
    void removeJobStops(db_id jobId);


signals:
    void lineAdded(db_id lineId);
    void lineNameChanged(db_id lineId);
    void lineStationsModified(db_id lineId);
    void lineAboutToBeRemoved(db_id lineId);
    void lineRemoved(db_id lineId);

    void stationAdded(db_id stationId);
    void stationNameChanged(db_id stationId);
    void stationModified(db_id stationId);
    void stationColorChanged(db_id stationId);
    void stationPlanChanged(db_id stationId);
    void stationRemoved(db_id stationId);

private slots:
    void onStationModified(db_id stId);
    void updateStationColor(db_id stationId);

private:
    sqlite3pp::database &mDb;
    LineStoragePrivate *impl;
};

#endif // LINESTORAGE_H
