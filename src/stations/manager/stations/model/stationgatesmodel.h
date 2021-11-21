#ifndef STATIONGATESMODEL_H
#define STATIONGATESMODEL_H

#include "utils/sqldelegate/pageditemmodelhelper.h"
#include "utils/sqldelegate/IFKField.h"

#include "stations/station_utils.h"

struct StationGatesModelItem
{
    db_id gateId;
    db_id defaultInPlatfId;
    int outTrackCount;
    QChar letter;
    QString defPlatfName;
    utils::Side side;
    QFlags<utils::GateType> type;
};

class StationGatesModel : public IPagedItemModelImpl<StationGatesModel, StationGatesModelItem>,
                          public IFKField
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    enum Columns {
        LetterCol = 0,
        SideCol,
        OutTrackCountCol,
        DefaultInPlatfCol,
        IsEntranceCol,
        IsExitCol,
        NCols
    };

    typedef StationGatesModelItem GateItem;
    typedef IPagedItemModelImpl<StationGatesModel, StationGatesModelItem> BaseClass;

    StationGatesModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // QAbstractTableModel

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &idx, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& idx) const override;

    // IPagedItemModel

    // Sorting TODO: enable multiple columns sort/filter with custom QHeaderView
    virtual void setSortingColumn(int col) override;

    // IFKField

    virtual bool getFieldData(int row, int col, db_id &idOut, QString& nameOut) const override;
    virtual bool validateData(int row, int col, db_id id, const QString& name) override;
    virtual bool setFieldData(int row, int col, db_id id, const QString& name) override;

    // StationGatesModel

    bool setStation(db_id stationId);
    inline db_id getStation() const { return m_stationId; }

    bool addGate(const QChar& name, db_id *outGateId = nullptr);
    bool removeGate(db_id gateId);

    inline bool isEditable() const { return editable; }
    inline void setEditable(bool val) { editable = val; }

    inline bool hasAtLeastOneGate()
    {
        refreshData(); //Recalc count
        return getTotalItemsCount() > 0;
    }

    QString getStationName() const;
    bool getStationInfo(QString &name, QString& shortName,
                        utils::StationType& type, qint64& phoneNumber,
                        bool &hasImage) const;
    bool setStationInfo(const QString &name, const QString& shortName, utils::StationType type, qint64 phoneNumber);

    // Convinience
    inline db_id getIdAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return 0; //Invalid

        const GateItem& item = cache.at(row - cacheFirstRow);
        return item.gateId;
    }

    inline QChar getNameAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return QChar(); //Invalid

        const GateItem& item = cache.at(row - cacheFirstRow);
        return item.letter;
    }

signals:
    void gateNameChanged(db_id gateId, const QString& name);
    void gateRemoved(db_id gateId);

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    friend BaseClass;
    Q_INVOKABLE void internalFetch(int firstRow, int sortCol, int valRow, const QVariant &val);

    bool setName(GateItem &item, const QChar &val);
    bool setSide(GateItem &item, int val);
    bool setType(GateItem &item, QFlags<utils::GateType> type);
    bool setOutTrackCount(GateItem &item, int count);
    bool setDefaultPlatf(StationGatesModel::GateItem &item, db_id trackId, const QString &trackName);

private:
    db_id m_stationId;
    bool editable;
};

#endif // STATIONGATESMODEL_H
