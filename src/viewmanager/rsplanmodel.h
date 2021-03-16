#ifndef RSPLANMODEL_H
#define RSPLANMODEL_H

#include <QAbstractTableModel>

#include <QVector>

#include <QTime>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

typedef struct RsPlanItem_
{
    db_id jobId;
    db_id stopId;
    db_id stationId;
    QTime arrival;
    QTime departure;
    QString stationName;
    JobCategory jobCat;
    RsOp op;
} RsPlanItem;

class RsPlanModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns
    {
        JobName = 0,
        Station,
        Arrival,
        Departure,
        Operation,
        NCols
    };

    RsPlanModel(sqlite3pp::database& db,
                QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void loadPlan(db_id rsId);
    void clear();

    RsPlanItem getItem(int row);

private:
    sqlite3pp::database& mDb;

    QVector<RsPlanItem> m_data;
};

#endif // RSPLANMODEL_H
