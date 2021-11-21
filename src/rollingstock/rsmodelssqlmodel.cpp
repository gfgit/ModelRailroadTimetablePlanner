#include "rsmodelssqlmodel.h"

#include "utils/sqldelegate/pageditemmodelhelper_impl.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/model_roles.h"
#include "utils/rs_types_names.h"

#include <QDebug>

static constexpr char
errorModelNameAlreadyUsedWithSameSuffix[] = QT_TRANSLATE_NOOP("RSModelsSQLModel",
                                                              "This model name (<b>%1</b>) is already used with the same"
                                                              " suffix (<b>%2</b>).\n"
                                                              "If you intend to create a new model of same name but different"
                                                              " suffix, please first set the suffix.");

static constexpr char
errorModelSuffixAlreadyUsedWithSameName[] = QT_TRANSLATE_NOOP("RSModelsSQLModel",
                                                              "This model suffix (<b>%1</b>) is already used with the same"
                                                              " name (<b>%2</b>).");

static constexpr char
errorSpeedMustBeGreaterThanZero[] = QT_TRANSLATE_NOOP("RSModelsSQLModel",
                                                      "Rollingstock maximum speed must be > 0 km/h.");

static constexpr char
errorAtLeastTwoAxes[] = QT_TRANSLATE_NOOP("RSModelsSQLModel",
                                          "Rollingstock must have at least 2 axes.");

static constexpr char
errorModelInUseCannotDelete[] = QT_TRANSLATE_NOOP("RSModelsSQLModel",
                                                  "There are rollingstock pieces of model <b>%1</b> so it cannot be removed.\n"
                                                  "If you wish to remove it, please first delete all <b>%1</b> pieces.");

RSModelsSQLModel::RSModelsSQLModel(sqlite3pp::database &db, QObject *parent) :
    BaseClass(500, db, parent)
{
    sortColumn = Name;
}

QVariant RSModelsSQLModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case Name:
                return RsTypeNames::tr("Name");
            case Suffix:
                return RsTypeNames::tr("Suffix");
            case MaxSpeed:
                return RsTypeNames::tr("Max Speed");
            case Axes:
                return RsTypeNames::tr("N. Axes");
            case TypeCol:
                return RsTypeNames::tr("Type");
            case SubTypeCol:
                return RsTypeNames::tr("Subtype");
            default:
                break;
            }
        }
        else
        {
            return section + curPage * ItemsPerPage + 1;
        }
    }
    return IPagedItemModel::headerData(section, orientation, role);
}

QVariant RSModelsSQLModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    //qDebug() << "Data:" << idx.row();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<RSModelsSQLModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const RSModel& item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case Name:
            return item.name;
        case Suffix:
            return item.suffix;
        case MaxSpeed:
            return QStringLiteral("%1 km/h").arg(item.maxSpeedKmH); //TODO: maybe QString('%1 km/h').arg(maxSpeed) AND custom spinBox with suffix
        case Axes:
            return int(item.axes);
        case TypeCol:
        {
            return RsTypeNames::name(item.type);
        }
        case SubTypeCol:
        {
            if(item.type != RsType::Engine)
                break;

            return RsTypeNames::name(item.sub_type);
        }
        }

        break;
    }
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case Name:
            return item.name;
        case Suffix:
            return item.suffix;
        case MaxSpeed:
            return item.maxSpeedKmH;
        case Axes:
            return int(item.axes);
        }

        break;
    }
    case Qt::TextAlignmentRole:
    {
        if(idx.column() == MaxSpeed || idx.column() == Axes)
            return Qt::AlignRight + Qt::AlignVCenter;
        break;
    }
    case RS_MODEL_ID:
        return item.modelId;
    case RS_TYPE_ROLE:
        return int(item.type);
    case RS_SUB_TYPE_ROLE:
        return int(item.sub_type);
    }

    return QVariant();
}

bool RSModelsSQLModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    const int row = idx.row();
    if(!idx.isValid() || row >= curItemCount || idx.column() >= NCols || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    RSModel &item = cache[row - cacheFirstRow];
    QModelIndex first = idx;
    QModelIndex last = idx;

    switch (role)
    {
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case Name:
        {
            const QString newName = value.toString().simplified();
            if(!setNameOrSuffix(item, newName, false))
                return false;
            break;
        }
        case Suffix:
        {
            const QString newName = value.toString().simplified();
            if(!setNameOrSuffix(item, newName, true))
                return false;
            break;
        }
        case MaxSpeed:
        {
            int speedKmH = value.toInt();
            if(speedKmH < 1)
            {
                emit modelError(tr(errorSpeedMustBeGreaterThanZero));
                return false;
            }

            if(item.maxSpeedKmH == speedKmH)
                return false; //No change

            command set_speed(mDb, "UPDATE rs_models SET max_speed=? WHERE id=?");
            set_speed.bind(1, speedKmH);
            set_speed.bind(2, item.modelId);
            if(set_speed.execute() != SQLITE_OK)
                return false;

            item.maxSpeedKmH = qint16(speedKmH);
            break;
        }
        case Axes:
        {
            int axes = value.toInt();
            if(axes < 2)
            {
                emit modelError(tr(errorAtLeastTwoAxes));
                return false;
            }

            if(item.axes == axes)
                return false; //No change

            command set_axes(mDb, "UPDATE rs_models SET axes=? WHERE id=?");
            set_axes.bind(1, axes);
            set_axes.bind(2, item.modelId);
            if(set_axes.execute() != SQLITE_OK)
                return false;

            item.axes = qint8(axes);
            break;
        }
        default:
            break;
        }
        break;
    }
    case RS_TYPE_ROLE: //Set through RS_TYPE_ROLE only
    {
        if(!setType(item, RsType(value.toInt()), RsEngineSubType::NTypes))
            return false;
        first = index(row, TypeCol);
        last = index(row, SubTypeCol);
        break;
    }
    case RS_SUB_TYPE_ROLE:
    {
        if(!setType(item, RsType::NTypes, RsEngineSubType(value.toInt())))
            return false;
        first = index(row, TypeCol);
        last = index(row, SubTypeCol);
        break;
    }
    default:
        break;
    }

    emit dataChanged(first, last);
    return true;
}

Qt::ItemFlags RSModelsSQLModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f; //Not fetched yet

    if(idx.column() != SubTypeCol || cache[idx.row() - cacheFirstRow].type == RsType::Engine)
        f.setFlag(Qt::ItemIsEditable); //NOTE: SubTypeCol is editable only fot Engines

    return f;
}

void RSModelsSQLModel::setSortingColumn(int col)
{
    if(sortColumn == col || (col != Name && col != TypeCol))
        return;

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

bool RSModelsSQLModel::removeRSModel(db_id modelId, const QString& name)
{
    if(!modelId)
        return false;

    command cmd(mDb, "DELETE FROM rs_models WHERE id=?");
    cmd.bind(1, modelId);
    int ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        ret = mDb.extended_error_code();
        if(ret == SQLITE_CONSTRAINT_TRIGGER)
        {
            QString tmp = name;
            if(name.isNull())
            {
                query q(mDb, "SELECT name FROM rs_models WHERE id=?");
                q.bind(1, modelId);
                q.step();
                tmp = q.getRows().get<QString>(0);
            }

            emit modelError(tr(errorModelInUseCannotDelete).arg(tmp));
            return false;
        }
        qWarning() << "RSModelsSQLModel: error removing model" << ret << mDb.error_msg();
        return false;
    }

    refreshData(); //Recalc row count
    return true;
}

bool RSModelsSQLModel::removeRSModelAt(int row)
{
    if(row >= curItemCount || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    const RSModel &item = cache.at(row - cacheFirstRow);

    return removeRSModel(item.modelId, item.name);
}

db_id RSModelsSQLModel::addRSModel(int *outRow, db_id sourceModelId, const QString& suffix, QString *errOut)
{
    db_id modelId = 0;

    command cmd(mDb);
    if(sourceModelId)
    {
        cmd.prepare("INSERT INTO rs_models(id,name,suffix,max_speed,axes,type,sub_type)"
                    "SELECT NULL,name,?,max_speed,axes,type,sub_type FROM rs_models WHERE id=?");
        cmd.bind(1, suffix);
        cmd.bind(2, sourceModelId);
    }else{
        cmd.prepare("INSERT INTO rs_models(id,name,suffix,max_speed,axes,type,sub_type) VALUES (NULL,'','',120,4,0,0)");
    }

    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    int ret = cmd.execute();
    if(ret == SQLITE_OK)
    {
        modelId = mDb.last_insert_rowid();
    }
    sqlite3_mutex_leave(mutex);

    if(outRow)
        *outRow = modelId ? 0 : -1; //Empty name is always the first

    if(ret == SQLITE_CONSTRAINT_UNIQUE)
    {
        if(sourceModelId)
        {
            //Error: suffix is already used
            if(errOut)
                *errOut = tr("Suffix is already used. Suffix must be different among models of same name.");
            return 0;
        }

        //There is already an empty model, use that instead
        query findEmpty(mDb, "SELECT id FROM rs_models WHERE name='' OR name IS NULL LIMIT 1");
        if(findEmpty.step() == SQLITE_ROW)
        {
            modelId = findEmpty.getRows().get<db_id>(0);
            if(outRow)
                *outRow = modelId ? 0 : -1; //Empty name is always the first
        }
    }
    else if(ret != SQLITE_OK)
    {
        QString msg = mDb.error_msg();
        if(errOut)
            *errOut = msg;
        qDebug() << "RS Model Error adding:" << ret << msg << mDb.error_code() << mDb.extended_error_code();
    }

    refreshData(); //Recalc row count
    switchToPage(0); //Reset to first page and so it is shown as first row

    return modelId;
}

db_id RSModelsSQLModel::getModelIdAtRow(int row) const
{
    if(row >= curItemCount || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return 0; //Not fetched yet or invalid
    const RSModel &item = cache.at(row - cacheFirstRow);
    return item.modelId;
}

bool RSModelsSQLModel::removeAllRSModels()
{
    command cmd(mDb, "DELETE FROM rs_models");
    int ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        qWarning() << "Removing ALL RS MODELS:" << ret << mDb.extended_error_code() << "Err:" << mDb.error_msg();
        return false;
    }

    refreshData(); //Recalc row count
    return true;
}

qint64 RSModelsSQLModel::recalcTotalItemCount()
{
    //TODO: consider filters
    query q(mDb, "SELECT COUNT(1) FROM rs_models");
    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

void RSModelsSQLModel::internalFetch(int first, int sortCol, int valRow, const QVariant& val)
{
    query q(mDb);

    int offset = first - valRow + curPage * ItemsPerPage;
    bool reverse = false;

    if(valRow > first)
    {
        offset = 0;
        reverse = true;
    }

    qDebug() << "Fetching:" << first << "ValRow:" << valRow << val << "Offset:" << offset << "Reverse:" << reverse;

    const char *whereCol = nullptr;

    QByteArray sql = "SELECT id,name,suffix,max_speed,axes,type,sub_type FROM rs_models";
    switch (sortCol)
    {
    case Name:
    {
        whereCol = "name,suffix"; //Order by 2 columns, no where clause
        break;
    }
    case TypeCol:
    {
        whereCol = "type,sub_type,name,suffix"; //Order by 4 columns, no where clause
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

    QVector<RSModel> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    int i = reverse ? BatchSize - 1 : 0;
    const int increment = reverse ? -1 : 1;

    for(; it != end; ++it)
    {
        auto r = *it;
        RSModel &item = vec[i];
        item.modelId = r.get<db_id>(0);
        item.name = r.get<QString>(1);
        item.suffix = r.get<QString>(2);
        item.maxSpeedKmH = r.get<qint16>(3);
        item.axes = r.get<qint8>(4);
        item.type = RsType(r.get<int>(5));
        item.sub_type = RsEngineSubType(r.get<int>(6));

        i += increment;
    }

    if(reverse && i > -1)
        vec.remove(0, i + 1);
    else if(i < BatchSize)
        vec.remove(i, BatchSize - i);

    postResult(vec, first);
}

bool RSModelsSQLModel::setNameOrSuffix(RSModel& item, const QString& newName, bool suffix)
{
    if(suffix ? item.suffix == newName : item.name == newName)
        return false; //No change

    command set_name(mDb, suffix ? "UPDATE rs_models SET suffix=? WHERE id=?"
                                 : "UPDATE rs_models SET name=? WHERE id=?");
    set_name.bind(1, newName);
    set_name.bind(2, item.modelId);
    int ret = set_name.execute();
    if(ret != SQLITE_OK)
    {
        qDebug() << "setNameOrSuffix()" << ret << mDb.error_code() << mDb.extended_error_code() << mDb.error_msg();
        ret = mDb.extended_error_code();
        if(ret == SQLITE_CONSTRAINT_UNIQUE)
        {
            emit modelError(tr(suffix ? errorModelSuffixAlreadyUsedWithSameName
                                      : errorModelNameAlreadyUsedWithSameSuffix)
                            .arg(newName)
                            .arg(suffix ? item.name : item.suffix));
        }
        return false;
    }

    if(suffix)
        item.suffix = newName;
    else
        item.name = newName;

    //This row has now changed position so we need to invalidate cache
    //HACK: we emit dataChanged for this index (that doesn't exist anymore)
    //but the view will trigger fetching at same scroll position so it is enough
    cache.clear();
    cacheFirstRow = 0;

    return true;
}

bool RSModelsSQLModel::setType(RSModel &item, RsType type, RsEngineSubType subType)
{
    if(type == RsType::NTypes)
        type = item.type;
    else
        subType = item.sub_type;

    if(type != RsType::Engine)
        subType = RsEngineSubType::Invalid; //Only engines can have a subType for now

    command set_type(mDb, "UPDATE rs_models SET type=?,sub_type=? WHERE id=?");
    set_type.bind(1, int(type));
    set_type.bind(2, int(subType));
    set_type.bind(3, item.modelId);
    if(set_type.execute() != SQLITE_OK)
        return false;

    item.type = type;
    item.sub_type = subType;

    if(sortColumn == TypeCol)
    {
        //This row has now changed position so we need to invalidate cache
        //HACK: we emit dataChanged for this index (that doesn't exist anymore)
        //but the view will trigger fetching at same scroll position so it is enough
        cache.clear();
        cacheFirstRow = 0;
    }

    return true;
}
