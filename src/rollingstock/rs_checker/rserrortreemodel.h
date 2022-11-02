#ifndef RSERRORTREEMODEL_H
#define RSERRORTREEMODEL_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include "rs_error_data.h"
#include "utils/singledepthtreemodelhelper.h"

class RsErrorTreeModel;
typedef SingleDepthTreeModelHelper<RsErrorTreeModel, RsErrors::RSErrorMap, RsErrors::RSErrorData> RsErrorTreeModelBase;

//TODO: make on-demand
class RsErrorTreeModel : public RsErrorTreeModelBase
{
    Q_OBJECT
public:
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

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void setErrors(const QMap<db_id, RsErrors::RSErrorList> &data);

    void mergeErrors(const QMap<db_id, RsErrors::RSErrorList> &data);

    void clear();

private slots:
    void onRSInfoChanged(db_id rsId);
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // RSERRORTREEMODEL_H
