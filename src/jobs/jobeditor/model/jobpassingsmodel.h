#ifndef JOBPASSINGSMODEL_H
#define JOBPASSINGSMODEL_H

#include <QAbstractTableModel>

#include <QVector>

#include <QTime>

#include "utils/types.h"

class JobPassingsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns {
        JobNameCol = 0,
        ArrivalCol,
        DepartureCol,
        PlatformCol,
        NCols
    };

    struct Entry
    {
        db_id jobId;
        QTime arrival;
        QTime departure;
        QString platform;
        JobCategory category;
    };

    explicit JobPassingsModel(QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void setJobs(const QVector<Entry>& vec);

private:
    QVector<Entry> m_data;
};

#endif // JOBPASSINGSMODEL_H
