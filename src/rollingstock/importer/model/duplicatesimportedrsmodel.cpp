#include "duplicatesimportedrsmodel.h"

#include "rollingstock/importer/intefaces/icheckname.h"

#include "utils/thread/iquittabletask.h"
#include "../intefaces/iduplicatesitemevent.h"

#include "../rsimportstrings.h"
#include "utils/rs_utils.h"

#include <QBrush>

#include <QElapsedTimer>


class DuplicatesImportedRSModelTask : public IQuittableTask
{
public:
    QVector<DuplicatesImportedRSModel::DuplicatedItem> items;

    inline DuplicatesImportedRSModelTask(int mode, sqlite3pp::database &db, QObject *receiver) :
        IQuittableTask(receiver),
        mDb(db)
    {
        fixItemsWithSameValues = (mode == IDuplicatesItemModel::FixingSameValues);
    }

    void run() override
    {
        timer.start();

        int count = -1;
        IDuplicatesItemModel::State state = IDuplicatesItemModel::CountingItems;

        if(wasStopped())
        {
            sendEvent(new IDuplicatesItemEvent(this,
                                               IDuplicatesItemEvent::ProgressAbortedByUser,
                                               IDuplicatesItemEvent::ProgressMaxFinished,
                                               count, state),
                      true);
            return;
        }

        //Inform model that task is started
        int max = 100;
        int progress = 0;
        sendEvent(new IDuplicatesItemEvent(this, progress++, max, count, state), false);

        if(fixItemsWithSameValues)
            execFixItemsWithSameValues();

        //Count how many items
        query q(mDb, "SELECT COUNT(1) FROM ("
                     "SELECT (CASE WHEN imp.new_number NOT NULL THEN imp.new_number ELSE imp.number END) AS num,"
                     " (CASE WHEN dup.new_number NOT NULL THEN dup.new_number ELSE dup.number END) AS dup_num"
                     " FROM imported_rs_list imp"
                     " JOIN imported_rs_models mod1 ON mod1.id=imp.model_id"
                     " JOIN imported_rs_owners own1 ON own1.id=imp.owner_id"

                     " LEFT JOIN imported_rs_list dup ON dup.id<>imp.id"
                     " LEFT JOIN imported_rs_models mod2 ON mod2.id=dup.model_id"
                     " LEFT JOIN imported_rs_owners own2 ON own2.id=dup.owner_id"
                     " LEFT JOIN rs_list ON rs_list.model_id=mod1.match_existing_id"
                     " WHERE own1.import AND mod1.import AND imp.import=1 AND"
                     " ("
                     "  rs_list.number=num OR"
                     "  ("
                     "   ("
                     "     imp.model_id=dup.model_id OR"
                     "     ("
                     "       mod1.match_existing_id NOT NULL AND mod1.match_existing_id=mod2.match_existing_id"
                     "     )"
                     "   )"
                     "   AND num=dup_num AND own2.import=1 AND mod2.import=1 AND dup.import=1"
                     "  )"
                     " )"
                     " GROUP BY imp.id)");
        q.step();
        count = q.getRows().get<int>(0);
        if(wasStopped())
        {
            sendEvent(new IDuplicatesItemEvent(this,
                                               IDuplicatesItemEvent::ProgressAbortedByUser,
                                               IDuplicatesItemEvent::ProgressMaxFinished,
                                               count, state),
                      true);
            return;
        }

        if(count == 0)
        {
            //No data to load, finish
            state = IDuplicatesItemModel::Loaded;
            sendEvent(new IDuplicatesItemEvent(this, progress, IDuplicatesItemEvent::ProgressMaxFinished,
                                               count, state),
                      true);
            return;
        }

        //Now load data
        state = IDuplicatesItemModel::LoadingData;
        max = count + 10;
        progress = 10;

        //Do not send event if the process is fast
        if(timer.elapsed() > IDuplicatesItemEvent::MinimumMSecsBeforeFirstEvent)
            sendEvent(new IDuplicatesItemEvent(this, progress, max, count, state), false);

        items.reserve(count);

        q.prepare("SELECT imp.id, mod1.id, mod1.match_existing_id, mod1.type,"
                  " mod1.name, mod1.new_name, rs_models.name, rs_models.type,"
                  " imp.number, imp.new_number,"
                  " own1.name, own1.new_name, rs_owners.name,"
                  " (CASE WHEN imp.new_number NOT NULL THEN imp.new_number ELSE imp.number END) AS num,"
                  " (CASE WHEN dup.new_number NOT NULL THEN dup.new_number ELSE dup.number END) AS dup_num"
                  " FROM imported_rs_list imp"
                  " JOIN imported_rs_models mod1 ON mod1.id=imp.model_id"
                  " JOIN imported_rs_owners own1 ON own1.id=imp.owner_id"

                  " LEFT JOIN imported_rs_list dup ON dup.id<>imp.id"
                  " LEFT JOIN imported_rs_models mod2 ON mod2.id=dup.model_id"
                  " LEFT JOIN imported_rs_owners own2 ON own2.id=dup.owner_id"
                  " LEFT JOIN rs_list ON rs_list.model_id=mod1.match_existing_id"

                  " LEFT JOIN rs_owners ON rs_owners.id=own1.match_existing_id"
                  " LEFT JOIN rs_models ON rs_models.id=mod1.match_existing_id"

                  " WHERE own1.import AND mod1.import AND imp.import=1 AND"
                  " ("
                  "  rs_list.number=num OR"
                  "  ("
                  "   ("
                  "     imp.model_id=dup.model_id OR"
                  "     ("
                  "       mod1.match_existing_id NOT NULL AND mod1.match_existing_id=mod2.match_existing_id"
                  "     )"
                  "   )"
                  "   AND num=dup_num AND own2.import=1 AND mod2.import=1 AND dup.import=1"
                  "  )"
                  " )"
                  " GROUP BY imp.id"
                  " ORDER BY mod1.name, imp.number, own1.name");
        sqlite3_stmt *st = q.stmt();

        //Send about 5 progress events during loading (but process at least 5 items between 2 events)
        const int sentTreshold = qMax(5, max / 5);

        for(auto r : q)
        {
            if(progress % 8 == 0 && wasStopped())
            {
                sendEvent(new IDuplicatesItemEvent(this,
                                                   IDuplicatesItemEvent::ProgressAbortedByUser,
                                                   IDuplicatesItemEvent::ProgressMaxFinished,
                                                   count, state),
                          true);
                return;
            }

            if(progress % sentTreshold && timer.elapsed() > IDuplicatesItemEvent::MinimumMSecsBeforeFirstEvent)
            {
                //It's time to report our progress
                sendEvent(new IDuplicatesItemEvent(this, progress, max, count, state), false);
            }

            DuplicatesImportedRSModel::DuplicatedItem item;
            item.importedId = r.get<db_id>(0);
            item.importedModelId = r.get<db_id>(1);

            //Model
            item.matchExistingModelId = r.get<db_id>(2);
            item.type = RsType(r.get<int>(3));
            item.modelName = QByteArray(reinterpret_cast<char const*>(sqlite3_column_text(st, 4)),
                                        sqlite3_column_bytes(st, 4));
            if(r.column_type(5) != SQLITE_NULL)
            {
                // 'name (new_name)'
                item.modelName.append(" (", 2);
                item.modelName.append(reinterpret_cast<char const*>(sqlite3_column_text(st, 5)),
                                      sqlite3_column_bytes(st, 5));
                item.modelName.append(')');
            }

            if(r.column_type(6) != SQLITE_NULL)
            {
                // 'name (match_existing name)'
                QByteArray tmp = QByteArray::fromRawData(reinterpret_cast<char const*>(sqlite3_column_text(st, 6)),
                                                         sqlite3_column_bytes(st, 6));

                if(tmp != item.modelName)
                {
                    item.modelName.append(" (", 2);
                    item.modelName.append(tmp);
                    item.modelName.append(')');
                }
                //Prefer matched model type when available
                item.type = RsType(r.get<int>(7));
            }

            //Number
            item.number = r.get<int>(8);

            if(r.column_type(9) == SQLITE_NULL)
                item.new_number = -1;
            else
                item.new_number = r.get<int>(9);

            item.ownerName = QByteArray(reinterpret_cast<char const*>(sqlite3_column_text(st, 10)),
                                        sqlite3_column_bytes(st, 10));
            if(r.column_type(11) != SQLITE_NULL)
            {
                // 'name (new_name)'
                item.ownerName.append(" (", 2);
                item.ownerName.append(reinterpret_cast<char const*>(sqlite3_column_text(st, 11)),
                                      sqlite3_column_bytes(st, 11));
                item.ownerName.append(')');
            }

            if(r.column_type(12) != SQLITE_NULL)
            {
                // 'name (match_existing name)'
                QByteArray tmp = QByteArray::fromRawData(reinterpret_cast<char const*>(sqlite3_column_text(st, 12)),
                                                         sqlite3_column_bytes(st, 12));

                if(tmp != item.ownerName)
                {
                    item.ownerName.append(" (", 2);
                    item.ownerName.append(tmp);
                    item.ownerName.append(')');
                }
            }
            item.import = true; //Only imported items get selected so import is true

            items.append(item);

            progress++;
        }

        state = IDuplicatesItemModel::Loaded;
        sendEvent(new IDuplicatesItemEvent(this, progress, IDuplicatesItemEvent::ProgressMaxFinished,
                                           count, state),
                  true);
    }

    void execFixItemsWithSameValues()
    {
        //If 2 (or mode) rollingstock items have
        //same model and same number and same owner
        //pick one of them and discart the other(s)
        //TODO: implement and add button to FixDuplicatesDlg to trigger this mode
        //And re-evalue progress values
    }

private:
    sqlite3pp::database &mDb;
    QElapsedTimer timer;
    bool fixItemsWithSameValues;
};

DuplicatesImportedRSModel::DuplicatesImportedRSModel(database &db, ICheckName *i, QObject *parent):
    IDuplicatesItemModel(db, parent),
    iface(i)
{

}

QVariant DuplicatesImportedRSModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case Import:
            return RsImportStrings::tr("Import");
        case ModelName:
            return RsImportStrings::tr("Model");
        case Number:
            return RsImportStrings::tr("Number");
        case NewNumber:
            return RsImportStrings::tr("New number");
        case OwnerName:
            return RsImportStrings::tr("Owner");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

int DuplicatesImportedRSModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : items.size();
}

int DuplicatesImportedRSModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant DuplicatesImportedRSModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= items.size() || idx.column() >= NCols)
        return QVariant();

    const DuplicatedItem& item = items.at(idx.row());

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case ModelName:
            return item.modelName;
        case Number:
            return rs_utils::formatNum(item.type, item.number);
        case NewNumber:
            return item.new_number == -1 ? QVariant() : rs_utils::formatNum(item.type, item.new_number);
        case OwnerName:
            return item.ownerName;
        }
        break;
    }
    case Qt::EditRole:
    {
        if(idx.column() == NewNumber)
            return item.new_number;
        break;
    }
    case Qt::TextAlignmentRole:
    {
        if(idx.column() == Number || idx.column() == NewNumber)
            return Qt::AlignRight + Qt::AlignVCenter;
        break;
    }
    case Qt::BackgroundRole:
    {
        if(!item.import || (idx.column() == NewNumber && item.new_number == -1))
            return QBrush(Qt::lightGray);
        break;
    }
    case Qt::CheckStateRole:
    {
        switch (idx.column())
        {
        case Import:
            return item.import ? Qt::Checked : Qt::Unchecked;
        }
        break;
    }
    }
    return QVariant();
}

bool DuplicatesImportedRSModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (!idx.isValid() || idx.row() >= items.size())
        return false;

    DuplicatedItem& item =items[idx.row()];

    switch (role)
    {
    case Qt::EditRole:
    {
        if(idx.column() == NewNumber)
        {
            int newNumber = value.toInt();

            if(item.new_number == newNumber)
                return false;

            QString errText;
            if(!iface->checkNewNumberIsValid(item.importedId, item.importedModelId, item.matchExistingModelId,
                                             item.type, item.number, newNumber, &errText))
            {
                emit error(errText);
                return false;
            }

            command set_newNumber(mDb, "UPDATE imported_rs_list SET new_number=? WHERE id=?");
            if(newNumber == -1)
                set_newNumber.bind(1); //Bind NULL
            else
                set_newNumber.bind(1, newNumber);
            set_newNumber.bind(2, item.importedId);
            if(set_newNumber.execute() != SQLITE_OK)
                return false;

            item.new_number = newNumber;
            emit dataChanged(idx, idx);
        }
        break;
    }
    case Qt::CheckStateRole:
    {
        if(idx.column() == Import)
        {
            Qt::CheckState cs = value.value<Qt::CheckState>();
            const bool import = cs == Qt::Checked;
            if(item.import == import)
                return false; //No change

            if(import)
            {
                //Newly imported, check if there are duplicates
                QString errText;
                if(!iface->checkNewNumberIsValid(item.importedId, item.importedModelId, item.matchExistingModelId,
                                                 item.type, item.number, item.new_number, &errText))
                {
                    emit error(errText);
                    return false;
                }
            }

            command set_imported(mDb, "UPDATE imported_rs_list SET import=? WHERE id=?");
            set_imported.bind(1, import ? 1 : 0);
            set_imported.bind(2, item.importedId);
            if(set_imported.execute() != SQLITE_OK)
                return false;

            item.import = import;

            //Update all columns to update background
            QModelIndex first = index(idx.row(), 0);
            QModelIndex last = index(idx.row(), NCols - 1);
            emit dataChanged(first, last);
        }
        break;
    }
    }

    return true;
}

Qt::ItemFlags DuplicatesImportedRSModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
    if(idx.column() == NewNumber)
        f.setFlag(Qt::ItemIsEditable);
    else if(idx.column() == Import)
        f.setFlag(Qt::ItemIsUserCheckable);

    return f;
}

IQuittableTask *DuplicatesImportedRSModel::createTask(int mode)
{
    return new DuplicatesImportedRSModelTask(mode, mDb, this);
}

void DuplicatesImportedRSModel::handleResult(IQuittableTask *task)
{
    beginResetModel();
    items = static_cast<DuplicatesImportedRSModelTask *>(task)->items;
    endResetModel();
}
