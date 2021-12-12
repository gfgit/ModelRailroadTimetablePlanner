#ifdef ENABLE_RS_CHECKER

#include "rserrortreemodel.h"

#include <QCoreApplication> //For translations

#include "utils/jobcategorystrings.h"

static const char* error_texts[] = {
    nullptr,
    QT_TRANSLATE_NOOP("RsErrors", "Stop is transit. Cannot couple/uncouple rollingstock."),
    QT_TRANSLATE_NOOP("RsErrors", "Coupled while busy: it was already coupled to another job."),
    QT_TRANSLATE_NOOP("RsErrors", "Uncoupled when not coupled."),
    QT_TRANSLATE_NOOP("RsErrors", "Not uncoupled at the end of the job or coupled by another job before this jobs uncouples it."),
    QT_TRANSLATE_NOOP("RsErrors", "Coupled in a different station than that where it was uncoupled."),
    QT_TRANSLATE_NOOP("RsErrors", "Uncoupled in the same stop it was coupled.")
};

class RsError
{
    Q_DECLARE_TR_FUNCTIONS(RsErrors)
};

RsErrorTreeModel::RsErrorTreeModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    //FIXME: listen for changes in rs/job/station names and update them
}

QVariant RsErrorTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case JobName:
            return tr("Job");
        case StationName:
            return tr("Station");
        case Arrival:
            return tr("Arrival");
        case Description:
            return tr("Description");
        default:
            break;
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

QModelIndex RsErrorTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if(parent.isValid())
    {
        if(parent.row() >= m_data.size() || parent.internalPointer())
            return QModelIndex(); //Out of bound or child-most

        auto it = m_data.constBegin() + parent.row();
        if(row >= it->errors.size())
            return QModelIndex();

        void *ptr = const_cast<void *>(static_cast<const void *>(&it->errors.at(row)));
        return createIndex(row, column, ptr);
    }

    if(row >= m_data.size())
        return QModelIndex();

    return createIndex(row, column, nullptr);
}

QModelIndex RsErrorTreeModel::parent(const QModelIndex &idx) const
{
    if(!idx.isValid())
        return QModelIndex();

    RsErrors::ErrorData *item = static_cast<RsErrors::ErrorData*>(idx.internalPointer());
    if(!item) //Caption
        return QModelIndex();

    auto it = m_data.constFind(item->rsId);
    if(it == m_data.constEnd())
        return QModelIndex();

    int row = std::distance(m_data.constBegin(), it);
    return index(row, 0);
}

int RsErrorTreeModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
    {
        if(parent.internalPointer())
            return 0; //Child most

        auto it = m_data.constBegin() + parent.row();
        return it->errors.size();
    }

    return m_data.size();
}

int RsErrorTreeModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 1 : NCols;
}

bool RsErrorTreeModel::hasChildren(const QModelIndex &parent) const
{
    if(parent.isValid())
    {
        if(parent.internalPointer() || parent.row() >= m_data.size())
            return false;

        auto it = m_data.constBegin() + parent.row();
        return it->errors.size();
    }

    return m_data.size(); // size > 0
}

QModelIndex RsErrorTreeModel::sibling(int row, int column, const QModelIndex &idx) const
{
    if(!idx.isValid())
        return QModelIndex();

    if(idx.internalPointer())
    {
        return createIndex(row, column, idx.internalPointer());
    }

    if(column > 0 || row >= m_data.size())
        return QModelIndex();

    return createIndex(row, 0, nullptr);
}

QVariant RsErrorTreeModel::data(const QModelIndex &idx, int role) const
{
    if(!idx.isValid() || role != Qt::DisplayRole)
        return QVariant();

    RsErrors::ErrorData *item = static_cast<RsErrors::ErrorData*>(idx.internalPointer());
    if(item)
    {
        switch (idx.column())
        {
        case JobName:
            return JobCategoryName::jobName(item->job.jobId, item->job.category);
        case StationName:
            return item->stationName;
        case Arrival:
            return item->time;
        case Description:
            return RsError::tr(error_texts[item->errorType]);
        }
    }
    else
    {
        //Caption
        if(idx.row() >= m_data.size())
            return QVariant();

        auto it = m_data.constBegin() + idx.row();
        if(idx.column() == 0)
            return it->rsName;
    }

    return QVariant();
}

/* Description:
 * Clear current errors and set new ones
*/
void RsErrorTreeModel::setErrors(const QMap<db_id, RsErrors::RSErrorList> &data)
{
    beginResetModel();
    m_data = data;
    endResetModel();
}

/* Description:
 * Merge new errors with pre-existing.
 * - If an RS is passed with no errors (i.e. empty QVector) it gets removed from the model
 * - If an new RS is passed it gets inserted in the model
 * - If an RS already in the model is passed then its current errors are cleared and the new errors are inserted
*/
void RsErrorTreeModel::mergeErrors(const QMap<db_id, RsErrors::RSErrorList> &data)
{
    Data::iterator oldIter = m_data.begin();
    int row = 0;
    for(auto it = data.constBegin(); it != data.constEnd(); it++)
    {
        auto iter = m_data.find(it.key());
        if(iter == m_data.end()) //Insert a new RS
        {
            if(it->errors.isEmpty())
                continue; //Error: tried to remove an RS not in this model (maybe already removed)

            Data::iterator pos = m_data.lowerBound(it.key());
            row += std::distance(oldIter, pos);

            beginInsertRows(QModelIndex(), row, row);
            iter = m_data.insert(pos, it.key(), it.value());
            endInsertRows();
        }
        else
        {
            row += std::distance(oldIter, iter);

            if(it->errors.isEmpty()) //Remove RS
            {
                beginRemoveRows(QModelIndex(), row, row);
                iter = m_data.erase(iter);
                endRemoveRows();
            }
            else //Repopulate
            {
                //First remove old rows to invalidate QModelIndex because they store a pointer to vector elements that will become dangling
                QModelIndex parent = createIndex(row, 0, nullptr);
                beginRemoveRows(parent, 0, iter->errors.size() - 1);
                iter->errors.clear();
                endRemoveRows();

                beginInsertRows(parent, 0, it->errors.size() - 1);
                iter.value() = it.value(); //Copy errors QVector
                endInsertRows();
            }
        }

        oldIter = iter;
    }
}

void RsErrorTreeModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
}

RsErrors::ErrorData* RsErrorTreeModel::getItem(const QModelIndex& idx) const
{
    if(idx.internalPointer())
        return static_cast<RsErrors::ErrorData *>(idx.internalPointer());
    return nullptr;
}

void RsErrorTreeModel::onRSInfoChanged(db_id rsId)
{
    Q_UNUSED(rsId)
    //Update top-level items that show RS names
    if(m_data.size())
    {
        QModelIndex first = index(0, 0);
        QModelIndex last = index(m_data.size(), 0);

        emit dataChanged(first, last);
    }
}

#endif // ENABLE_RS_CHECKER
