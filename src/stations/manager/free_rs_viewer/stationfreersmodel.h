#ifndef STATIONFREERSMODEL_H
#define STATIONFREERSMODEL_H

#include <QAbstractTableModel>

#include <QVector>

#include <QTime>

#include <sqlite3pp/sqlite3pp.h>

#include "utils/types.h"

//TODO: on-demand load and let SQL do the sorting
class StationFreeRSModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns {
        RSNameCol = 0,
        FreeFromTimeCol,
        FreeUpToTimeCol,
        FromJobCol,
        ToJobCol,
        NCols
    };

    typedef struct Item_
    {
        db_id rsId = 0;
        QTime from; //Time at which RS is uncoupled (from now it's free)
        QTime to; //Time at which is coupled (not free anymore)
        QString name;
        db_id fromJob = 0;
        db_id fromStopId = 0;
        db_id toJob = 0;
        db_id toStopId = 0;
        JobCategory fromJobCat;
        JobCategory toJobCat;
    } Item;

    StationFreeRSModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    const Item *getItemAt(int row) const;

    void setStation(db_id stId);
    void setTime(QTime time);

    enum ErrorCodes
    {
        NoError = 0,
        NoOperationFound,
        DBError
    };

    ErrorCodes getNextOpTime(QTime &time);
    ErrorCodes getPrevOpTime(QTime& time);

    QTime getTime() const;
    db_id getStationId() const;

    QString getStationName() const;

    bool sortByColumn(int col);

    int getSortCol() const;

public slots:
    void reloadData();

private:
    db_id m_stationId;
    QTime m_time;
    int sortCol;

    QVector<Item> m_data;

    sqlite3pp::database& mDb;
};

#endif // STATIONFREERSMODEL_H
