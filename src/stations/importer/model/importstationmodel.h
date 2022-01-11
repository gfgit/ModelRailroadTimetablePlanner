#ifndef IMPORTSTATIONMODEL_H
#define IMPORTSTATIONMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"

#include "utils/types.h"
#include "stations/station_utils.h"

struct ImportStationModelItem
{
    db_id stationId;
    QString name;
    QString shortName;
    utils::StationType type;
};

class ImportStationModel : public IPagedItemModelImpl<ImportStationModel, ImportStationModelItem>
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    enum Columns{
        NameCol = 0,
        ShortNameCol,
        TypeCol,
        NCols
    };

    typedef ImportStationModelItem StationItem;
    typedef IPagedItemModelImpl<ImportStationModel, ImportStationModelItem> BaseClass;

    ImportStationModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // QAbstractTableModel

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // IPagedItemModel

    // Sorting
    virtual void setSortingColumn(int col) override;

    //Filter
    std::pair<QString, FilterFlags> getFilterAtCol(int col) override;
    bool setFilterAtCol(int col, const QString& str) override;

    // Convinience
    inline db_id getIdAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return 0; //Invalid

        const StationItem& item = cache.at(row - cacheFirstRow);
        return item.stationId;
    }

    inline QString getNameAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return QString(); //Invalid

        const StationItem& item = cache.at(row - cacheFirstRow);
        return item.name;
    }

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    friend BaseClass;
    Q_INVOKABLE void internalFetch(int firstRow, int sortCol, int valRow, const QVariant &val);

    QString m_nameFilter;
};

#endif // IMPORTSTATIONMODEL_H
