#ifndef DUPLICATESIMPORTEDRSMODEL_H
#define DUPLICATESIMPORTEDRSMODEL_H

#include <QAbstractTableModel>
#include <QVector>

#include "utils/types.h"

#include "../intefaces/iduplicatesitemmodel.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class ICheckName;

class DuplicatesImportedRSModel : public IDuplicatesItemModel
{
    Q_OBJECT

public:
    enum Columns
    {
        Import = 0,
        ModelName,
        Number,
        NewNumber,
        OwnerName,
        NCols
    };

    typedef struct
    {
        db_id importedId;
        db_id importedModelId;
        db_id matchExistingModelId;
        QByteArray modelName;
        QByteArray ownerName;
        int number;
        int new_number;
        RsType type;
        bool import;
    } DuplicatedItem;

    DuplicatesImportedRSModel(database &db, ICheckName *i, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &idx, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& idx) const override;

protected:
    // IDuplicatesItemModel
    IQuittableTask* createTask(int mode) override;
    void handleResult(IQuittableTask *task) override;

private:
    ICheckName *iface;

    QVector<DuplicatedItem> items;
};

#endif // DUPLICATESIMPORTEDRSMODEL_H
