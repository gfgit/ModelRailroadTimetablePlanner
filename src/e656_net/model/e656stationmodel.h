#ifndef E656STATIONMODEL_H
#define E656STATIONMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QTime>

#include "e656_utils.h"

class E656NetImporter;

namespace sqlite3pp {
class database;
}

class E656StationModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns
    {
        JobCatNameCol = 0,
        JobNumber,
        JobMatchCat,
        PlatfCol,
        ArrivalCol,
        DepartureCol,
        OriginCol,
        DestinationCol,
        NCols
    };

    E656StationModel(sqlite3pp::database &db, QObject *parent = nullptr);
    ~E656StationModel();

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &p = QModelIndex()) const override;
    int columnCount(const QModelIndex &p = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &idx, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &idx) const override;

    void setJobs(const QVector<ImportedJobItem>& jobs);

    void importSelectedJobs(E656NetImporter *importer);

private:
    QVector<ImportedJobItem> m_data;
};

#endif // E656STATIONMODEL_H
