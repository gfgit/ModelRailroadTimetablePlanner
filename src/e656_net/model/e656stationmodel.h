#ifndef E656STATIONMODEL_H
#define E656STATIONMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QTime>

#include "e656_utils.h"

class E656StationModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns
    {
        JobCatName = 0,
        JobNumber,
        JobMatchCat,
        PlatfCol,
        ArrivalCol,
        DepartureCol,
        OriginCol,
        DestinationCol,
        NCols
    };

    explicit E656StationModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &p = QModelIndex()) const override;
    int columnCount(const QModelIndex &p = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void setJobs(const QVector<ImportedJobItem>& jobs);

private:
    QVector<ImportedJobItem> m_data;
};

#endif // E656STATIONMODEL_H
