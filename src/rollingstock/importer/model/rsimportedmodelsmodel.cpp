#include "rsimportedmodelsmodel.h"

#include "../rsimportstrings.h"

#include <QEvent>
#include "utils/worker_event_types.h"

#include <QCoreApplication>

#include <QBrush>

#include "utils/rs_types_names.h"

#include <QDebug>

class ModelsResultEvent : public QEvent
{
public:
    static constexpr Type _Type = Type(CustomEvents::RsImportedModelsResult);
    inline ModelsResultEvent() : QEvent(_Type) {}

    QVector<RSImportedModelsModel::ModelItem> items;
    int firstRow;
};

RSImportedModelsModel::RSImportedModelsModel(database &db, QObject *parent) :
    IRsImportModel(db, parent),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize)
{
    sortColumn = Name;
}

bool RSImportedModelsModel::event(QEvent *e)
{
    if(e->type() == ModelsResultEvent::_Type)
    {
        ModelsResultEvent *ev = static_cast<ModelsResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return QAbstractTableModel::event(e);
}

void RSImportedModelsModel::fetchRow(int row)
{
    if(row >= firstPendingRow && row < firstPendingRow + BatchSize)
        return; //Already fetching

    if(row >= cacheFirstRow && row < cacheFirstRow + cache.size())
        return; //Already cached

    //TODO: abort fetching here

    const int remainder = row % BatchSize;
    firstPendingRow = row - remainder;
    qDebug() << "Requested:" << row << "From:" << firstPendingRow;

    QVariant val;
    int valRow = 0;
    ModelItem *item = nullptr;

    if(cache.size())
    {
        if(firstPendingRow >= cacheFirstRow + cache.size())
        {
            valRow = cacheFirstRow + cache.size();
            item = &cache.last();
        }
        else if(firstPendingRow > (cacheFirstRow - firstPendingRow))
        {
            valRow = cacheFirstRow;
            item = &cache.first();
        }
    }

    switch (sortColumn)
    {
    case Name:
    {
        if(item)
        {
            val = item->name;
        }
        break;
    }
        //No data hint for other columns
    }

    //TODO: use a custom QRunnable
    QMetaObject::invokeMethod(this, "internalFetch", Qt::QueuedConnection,
                              Q_ARG(int, firstPendingRow), Q_ARG(int, sortColumn),
                              Q_ARG(int, valRow), Q_ARG(QVariant, val));
}

void RSImportedModelsModel::internalFetch(int first, int sortCol, int valRow, const QVariant& val)
{
    query q(mDb);

    int offset = first - valRow;
    bool reverse = false;

    if(valRow > first)
    {
        offset = 0;
        reverse = true;
    }

    //FIXME: maybe show cyan background if there aren't RS of this model (like if owner or RS is not imported), and do the same for owners

    qDebug() << "Fetching:" << first << "ValRow:" << valRow << val << "Offset:" << offset << "Reverse:" << reverse;

    const char *whereCol;

    QByteArray sql = "SELECT imp.id,imp.name,imp.suffix,imp.import,imp.new_name,imp.match_existing_id,rs_models.name,"
                     " imp.max_speed,imp.axes,imp.type,imp.sub_type"
                     " FROM imported_rs_models imp LEFT JOIN rs_models ON rs_models.id=imp.match_existing_id";
    switch (sortCol)
    {
    case Name:
    {
        whereCol = "imp.name";
        break;
    }
    case Import:
    {
        whereCol = "imp.import DESC, imp.name"; //Order by 2 columns, no where clause
        break;
    }
    case MaxSpeedCol:
    {
        whereCol = "imp.max_speed";
        break;
    }
    case AxesCol:
    {
        whereCol = "imp.axes";
        break;
    }
    case TypeCol:
    {
        whereCol = "imp.type,imp.sub_type"; //Order by 2 columns, no where clause
        break;
    }
    }

    if(val.isValid())
    {
        sql += " WHERE ";
        sql += whereCol;
        if(reverse)
            sql += "<?3";
        else
            sql += ">?3";
    }

    sql += " ORDER BY ";
    sql += whereCol;

    if(reverse)
        sql += " DESC";

    sql += " LIMIT ?1";
    if(offset)
        sql += " OFFSET ?2";

    q.prepare(sql);
    q.bind(1, BatchSize);
    if(offset)
        q.bind(2, offset);

    if(val.isValid())
    {
        switch (sortCol)
        {
        case Name:
        {
            q.bind(3, val.toString());
            break;
        }
        }
    }

    QVector<ModelItem> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    if(reverse)
    {
        int i = BatchSize - 1;

        for(; it != end; ++it)
        {
            auto r = *it;
            ModelItem &item = vec[i];
            item.importdModelId = r.get<db_id>(0);
            item.name = r.get<QString>(1);
            item.suffix = r.get<QString>(2);
            item.import = r.get<int>(3) != 0;
            if(r.column_type(4) != SQLITE_NULL)
                item.customName = r.get<QString>(4);
            item.matchExistingId = r.get<db_id>(5);
            if(item.matchExistingId)
                item.matchExistingName = r.get<QString>(6);
            item.maxSpeedKmH = qint16(r.get<int>(7));
            item.axes = qint8(r.get<int>(8));
            item.type = RsType(r.get<int>(9));
            item.subType = RsEngineSubType(r.get<int>(10));
            i--;
        }
        if(i > -1)
            vec.remove(0, i + 1);
    }
    else
    {
        int i = 0;

        for(; it != end; ++it)
        {
            auto r = *it;
            ModelItem &item = vec[i];
            item.importdModelId = r.get<db_id>(0);
            item.name = r.get<QString>(1);
            item.suffix = r.get<QString>(2);
            item.import = r.get<int>(3) != 0;
            if(r.column_type(4) != SQLITE_NULL)
                item.customName = r.get<QString>(4);
            item.matchExistingId = r.get<db_id>(5);
            if(item.matchExistingId)
                item.matchExistingName = r.get<QString>(6);
            item.maxSpeedKmH = qint16(r.get<int>(7));
            item.axes = qint8(r.get<int>(8));
            item.type = RsType(r.get<int>(9));
            item.subType = RsEngineSubType(r.get<int>(10));
            i++;
        }
        if(i < BatchSize)
            vec.remove(i, BatchSize - i);
    }


    ModelsResultEvent *ev = new ModelsResultEvent;
    ev->items = vec;
    ev->firstRow = first;

    qApp->postEvent(this, ev);
}

void RSImportedModelsModel::handleResult(const QVector<ModelItem>& items, int firstRow)
{
    if(firstRow == cacheFirstRow + cache.size())
    {
        qDebug() << "RES: appending First:" << cacheFirstRow;
        cache.append(items);
        if(cache.size() > ItemsPerPage)
        {
            const int extra = cache.size() - ItemsPerPage; //Round up to BatchSize
            const int remainder = extra % BatchSize;
            const int n = remainder ? extra + BatchSize - remainder : extra;
            qDebug() << "RES: removing last" << n;
            cache.remove(0, n);
            cacheFirstRow += n;
        }
    }
    else
    {
        if(firstRow + items.size() == cacheFirstRow)
        {
            qDebug() << "RES: prepending First:" << cacheFirstRow;
            QVector<ModelItem> tmp = items;
            tmp.append(cache);
            cache = tmp;
            if(cache.size() > ItemsPerPage)
            {
                const int n = cache.size() - ItemsPerPage;
                cache.remove(ItemsPerPage, n);
                qDebug() << "RES: removing first" << n;
            }
        }
        else
        {
            qDebug() << "RES: replacing";
            cache = items;
        }
        cacheFirstRow = firstRow;
        qDebug() << "NEW First:" << cacheFirstRow;
    }

    firstPendingRow = -BatchSize;

    QModelIndex firstIdx = index(firstRow, 0);
    QModelIndex lastIdx = index(firstRow + items.count() - 1, NCols - 1);
    emit dataChanged(firstIdx, lastIdx);

    qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}

QVariant RSImportedModelsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case Import:
            return RsImportStrings::tr("Import");
        case Name:
            return RsImportStrings::tr("Name");
        case CustomName:
            return RsImportStrings::tr("Custom Name");
        case MatchExisting:
            return RsImportStrings::tr("Match Existing");
        case SuffixCol:
            return RsTypeNames::tr("Suffix");
        case MaxSpeedCol:
            return RsTypeNames::tr("Max Speed");
        case AxesCol:
            return RsTypeNames::tr("Axes");
        case TypeCol:
            return RsTypeNames::tr("Type");
        case SubTypeCol:
            return RsTypeNames::tr("Sub Type");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

int RSImportedModelsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int RSImportedModelsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant RSImportedModelsModel::data(const QModelIndex &idx, int role) const
{
    int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    //qDebug() << "Data:" << idx.row();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<RSImportedModelsModel *>(this)->fetchRow(row);

        //Temporarily return null
        return QVariant("...");
    }

    const ModelItem& item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case Name:
            return item.name;
        case CustomName:
            return item.customName;
        case MatchExisting:
            return item.matchExistingName;
        case SuffixCol:
            return item.suffix;
        case MaxSpeedCol:
            return item.maxSpeedKmH;
        case AxesCol:
            return item.axes;
        case TypeCol:
            return RsTypeNames::name(item.type);
        case SubTypeCol:
            if(item.type != RsType::Engine)
                break;
            return RsTypeNames::name(item.subType);
        }
        break;
    }
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case CustomName:
            return item.customName;
        }
        break;
    }
    case Qt::BackgroundRole:
    {
        if(!item.import ||
                (idx.column() == CustomName && item.customName.isEmpty()) ||
                (idx.column() == MatchExisting && item.matchExistingId == 0))
            return QBrush(Qt::lightGray); //If not imported mark background or no custom/matching name set
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

bool RSImportedModelsModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    const int row = idx.row();
    if(!idx.isValid() || row >= curItemCount || idx.column() >= NCols || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    ModelItem &item = cache[row - cacheFirstRow];
    QModelIndex first = idx;
    QModelIndex last = idx;

    switch (role)
    {
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case CustomName:
        {
            const QString newName = value.toString().simplified();
            if(item.customName == newName)
                return false; //No change

            QString errText;
            if(!checkCustomNameValid(item.importdModelId, item.name, newName, &errText))
            {
                emit modelError(errText);
                return false;
            }

            command set_name(mDb, "UPDATE imported_rs_models SET new_name=?,match_existing_id=NULL WHERE id=?");

            if(newName.isEmpty())
                set_name.bind(1); //Bind NULL
            else
                set_name.bind(1, newName);
            set_name.bind(2, item.importdModelId);
            if(set_name.execute() != SQLITE_OK)
                return false;

            item.customName = newName;
            item.matchExistingId = 0;
            item.matchExistingName.clear();
            item.matchExistingName.squeeze();

            last = index(row, MatchExisting);

            break;
        }
        default:
            return false;
        }
        break;
    }
    case Qt::CheckStateRole:
    {
        switch (idx.column())
        {
        case Import:
        {
            Qt::CheckState cs = value.value<Qt::CheckState>();
            const bool import = cs == Qt::Checked;
            if(item.import == import)
                return false; //No change

            if(import)
            {
                //Newly imported, check for duplicates
                QString errText;
                if(!checkCustomNameValid(item.importdModelId, item.name, item.customName, &errText))
                {
                    emit modelError(errText);
                    return false;
                }
            }

            command set_imported(mDb, "UPDATE imported_rs_models SET import=? WHERE id=?");
            set_imported.bind(1, import ? 1 : 0);
            set_imported.bind(2, item.importdModelId);
            if(set_imported.execute() != SQLITE_OK)
                return false;

            item.import = import;

            if(sortColumn == Import)
            {
                //This row has now changed position so we need to invalidate cache
                //HACK: we emit dataChanged for this index (that doesn't exist anymore)
                //but the view will trigger fetching at same scroll position so it is enough
                cache.clear();
                cacheFirstRow = 0;
            }

            emit importCountChanged();

            //Update all columns to update background
            first = index(row, 0);
            last = index(row, NCols - 1);
            break;
        }
        default:
            return false;
        }
        break;
    }
    default:
        return false;
    }

    emit dataChanged(first, last);
    return true;
}

Qt::ItemFlags RSImportedModelsModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f; //Not fetched yet

    if(idx.column() == Import)
        f.setFlag(Qt::ItemIsUserCheckable);
    if(idx.column() == CustomName || idx.column() == MatchExisting)
        f.setFlag(Qt::ItemIsEditable);

    return f;
}

/* ISqlOnDemandModel */

qint64 RSImportedModelsModel::recalcTotalItemCount()
{
    query q(mDb, "SELECT COUNT(1) FROM imported_rs_models");
    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

void RSImportedModelsModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void RSImportedModelsModel::setSortingColumn(int col)
{
    if(sortColumn == col || col == CustomName || col == MatchExisting || col == SuffixCol || col == SubTypeCol)
        return; //Do not sort by CustomName or MatchExisting (not useful and complicated)

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

/* IRsImportModel */

int RSImportedModelsModel::countImported()
{
    query q(mDb, "SELECT COUNT(1) FROM imported_rs_models WHERE import=1");
    q.step();
    const int count = q.getRows().get<int>(0);
    return count;
}

/* ICheckName */

bool RSImportedModelsModel::checkCustomNameValid(db_id importedModelId, const QString& originalName, const QString& newCustomName, QString *errTextOut)
{
    if(originalName == newCustomName)
    {
        if(errTextOut)
        {
            if(originalName.isEmpty())
            {
                *errTextOut = tr("Models with empty name must have a Custom Name or must be matched to an existing model");
            }else{
                *errTextOut = tr("You cannot set the same name in the 'Custom Name' field.\n"
                                 "If you meant to revert to original name then clear the custom name and leave the cell empty");
            }
        }
        return false;
    }

    //FIXME: maybe use EXISTS instead of WHERE for performance

    //First check for duplicates
    query q_nameDuplicates(mDb, "SELECT id FROM rs_models WHERE name=? LIMIT 1");

    //If removing custom name check against original sheet name
    QString nameToCheck = newCustomName;
    if(newCustomName.isEmpty())
        nameToCheck = originalName;

    q_nameDuplicates.bind(1, nameToCheck);

    if(q_nameDuplicates.step() == SQLITE_ROW)
    {
        db_id modelId = q_nameDuplicates.getRows().get<db_id>(0);
        Q_UNUSED(modelId) //TODO: maybe use it?

        if(errTextOut)
        {
            *errTextOut = tr("There is already an existing Model with same name: <b>%1</b><br>"
                             "If you meant to merge theese rollingstock pieces with this existing model "
                             "please use 'Match Existing' field")
                    .arg(nameToCheck);
        }
        return false;
    }

    //Check also against other imported models original names (that don't have a custom name)
    q_nameDuplicates.prepare("SELECT id FROM imported_rs_models WHERE name=? AND new_name IS NULL AND id<>? LIMIT 1");
    q_nameDuplicates.bind(1, nameToCheck);
    q_nameDuplicates.bind(2, importedModelId);
    if(q_nameDuplicates.step() == SQLITE_ROW)
    {
        db_id modelId = q_nameDuplicates.getRows().get<db_id>(0);
        Q_UNUSED(modelId) //TODO: maybe use it?

        if(errTextOut)
        {
            *errTextOut = tr("There is already an imported Model with name: <b>%1</b><br>"
                             "If you meant to merge theese rollingstock pieces with this existing model "
                             "after importing rollingstock use the merge tool to merge them")
                    .arg(nameToCheck);
        }
        return false;
    }

    //Check also against other imported models custom names
    q_nameDuplicates.prepare("SELECT id, name FROM imported_rs_models WHERE new_name=? AND id<>? LIMIT 1");
    q_nameDuplicates.bind(1, nameToCheck);
    q_nameDuplicates.bind(2, importedModelId);
    if(q_nameDuplicates.step() == SQLITE_ROW)
    {
        db_id modelId = q_nameDuplicates.getRows().get<db_id>(0);
        Q_UNUSED(modelId) //TODO: maybe use it?

        QString otherOriginalName = q_nameDuplicates.getRows().get<QString>(1);

        if(newCustomName.isEmpty())
        {
            if(errTextOut)
            {
                *errTextOut = tr("You already gave the same custom name: <b>%1</b><br>"
                                 "to the imported model: <b>%2</b><br>"
                                 "In order to proceed you need to assign a different custom name to %2")
                                  .arg(nameToCheck, otherOriginalName);
            }
        }
        else
        {
            if(errTextOut)
            {
                *errTextOut = tr("You already gave the same custom name: <b>%1</b><br>"
                                 "to the imported model: <b>%2</b><br>"
                                 "Please choose a different name or leave empty for the original name")
                                  .arg(nameToCheck, otherOriginalName);
            }
        }

        return false;
    }

    return true;
}

/* IFKField */

bool RSImportedModelsModel::getFieldData(int row, int /*col*/, db_id &modelIdOut, QString &modelNameOut) const
{
    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false;

    const ModelItem& item = cache.at(row - cacheFirstRow);
    modelIdOut = item.matchExistingId;
    modelNameOut = item.matchExistingName;

    return true;
}

bool RSImportedModelsModel::validateData(int /*row*/, int /*col*/, db_id /*modelId*/, const QString &/*modelName*/)
{
    return true; //TODO: implement
}

bool RSImportedModelsModel::setFieldData(int row, int /*col*/, db_id modelId, const QString &modelName)
{
    //NOTE: CustomName and MatchExisting exclude each other
    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false;

    ModelItem& item = cache[row - cacheFirstRow];
    if(item.matchExistingId == modelId)
        return true; //No change

    if(modelId == 0)
    {
        //Check if we can leave it with no match and no custom name
        //FIXME: if owner is matched and instead user wants to set a custom name
        //but you can't remove the match because the name is a duplicate
        //The user should set a custom name, it will automatically remove the match
        //BUT we need to show a proper error test if just removing the match
        //so add it to checkCustomNameValid() with like 'bool isMatchingOwner=true' argument
        QString errText;
        if(!checkCustomNameValid(item.importdModelId, item.name, QString(), &errText))
        {
            emit modelError(errText);
            return false;
        }
    }

    command set_match(mDb, "UPDATE imported_rs_models SET new_name=NULL,match_existing_id=? WHERE id=?");
    if(modelId)
        set_match.bind(1, modelId);
    else
        set_match.bind(1); //Bind NULL
    set_match.bind(2, item.importdModelId);

    if(set_match.execute() != SQLITE_OK)
        return false;

    item.matchExistingId = modelId;
    item.matchExistingName = modelName;
    item.customName.clear();
    item.customName.squeeze();

    emit dataChanged(index(row, CustomName), index(row, MatchExisting));

    return true;
}
