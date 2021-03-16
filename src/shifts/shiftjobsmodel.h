#ifndef SHIFTJOBSMODEL_H
#define SHIFTJOBSMODEL_H

#include <QAbstractTableModel>

#include <QVector>

#include <QTime>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

typedef struct ShiftJobItem_
{
    db_id jobId;
    db_id originStId;
    db_id destinationStId;
    QTime start;
    QTime end;
    JobCategory cat;
    QString originStName;
    QString desinationStName;
} ShiftJobItem;

class ShiftJobsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns
    {
        JobName,
        Departure,
        Origin,
        Arrival,
        Destination,
        NCols
    };

    ShiftJobsModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void loadShiftJobs(db_id shiftId);

    db_id getJobAt(int row);

private:
    sqlite3pp::database& mDb;

    QVector<ShiftJobItem> m_data;
};

#endif // SHIFTJOBSMODEL_H
