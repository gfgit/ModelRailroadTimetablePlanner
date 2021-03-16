#ifndef DUPLICATESIMPORTEDITEMSMODEL_H
#define DUPLICATESIMPORTEDITEMSMODEL_H

#include <QAbstractTableModel>
#include <QVector>

#include "utils/types.h"
#include "utils/model_mode.h"

#include "../intefaces/iduplicatesitemmodel.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class ICheckName;

class DuplicatesImportedItemsModel : public IDuplicatesItemModel
{
    Q_OBJECT

public:
    enum{
        NModes = 2
    };

    enum Columns
    {
        Import = 0,
        SheetIdx,
        Name,
        CustomName,
        NCols
    };

    typedef struct
    {
        db_id importedId;
        QString originalName;
        QString customName;
        int sheetIdx;
        bool import;
    } DuplicatedItem;

    DuplicatesImportedItemsModel(ModelModes::Mode mode, database &db, ICheckName *i, QObject *parent = nullptr);

    // QAbstractTableModel

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

    ModelModes::Mode m_mode;
};

#endif // DUPLICATESIMPORTEDITEMSMODEL_H
