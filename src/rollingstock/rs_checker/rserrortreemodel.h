#ifndef RSERRORTREEMODEL_H
#define RSERRORTREEMODEL_H

#ifdef ENABLE_RS_CHECKER

#include <QAbstractItemModel>

#include "error_data.h"

//TODO: make on-demand
class RsErrorTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    typedef QMap<db_id, RsErrors::RSErrorList> Data;

    enum Columns
    {
        JobName = 0,
        StationName,
        Arrival,
        Description,
        NCols
    };

    RsErrorTreeModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &idx) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void setErrors(const QMap<db_id, RsErrors::RSErrorList> &data);

    void mergeErrors(const QMap<db_id, RsErrors::RSErrorList> &data);

    void clear();

    RsErrors::ErrorData *getItem(const QModelIndex &idx) const;

private slots:
    void onRSInfoChanged(db_id rsId);

private:
    Data m_data;
};

#endif // ENABLE_RS_CHECKER

#endif // RSERRORTREEMODEL_H
