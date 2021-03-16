#include "checkproxymodel.h"

CheckProxyModel::CheckProxyModel(QObject *parent) :
    QAbstractListModel(parent),
    sourceModel(nullptr),
    atLeastACheck(false)
{

}

QVariant CheckProxyModel::data(const QModelIndex &index, int role) const
{
    if(index.isValid() && index.row() < rowCount() && sourceModel)
    {
        if(role == Qt::CheckStateRole)
        {
            return m_checks.at(index.row()) ? Qt::Checked : Qt::Unchecked;
        }
        return sourceModel->data(index, role);
    }
    return QVariant();
}

bool CheckProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(index.isValid() && index.row() < rowCount() && role == Qt::CheckStateRole)
    {
        bool val = value.value<Qt::CheckState>() == Qt::Checked;

        m_checks[index.row()] = val;
        //If it has already other checks don't emit signal
        if(val && !atLeastACheck)
        {
            //New check
            atLeastACheck = true;
            emit hasCheck(true);
        }
        else if(atLeastACheck && !m_checks.contains(true))
        {
            //Had a check but now it doesn't anymore
            atLeastACheck = false;
            emit hasCheck(false);
        }

        emit dataChanged(index, index);
        return true;
    }
    return false;
}

int CheckProxyModel::rowCount(const QModelIndex &parent) const
{
    return sourceModel ? sourceModel->rowCount(parent) : 0;
}

Qt::ItemFlags CheckProxyModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren;
}

QVariant CheckProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(sourceModel)
        return sourceModel->headerData(section, orientation, role);
    return QVariant();
}

void CheckProxyModel::setSourceModel(QAbstractItemModel *value)
{
    beginResetModel();

    if(sourceModel)
    {
        disconnect(sourceModel, &QAbstractItemModel::dataChanged, this, &CheckProxyModel::onSourceDataChanged);
        disconnect(sourceModel, &QAbstractItemModel::headerDataChanged, this, &QAbstractItemModel::headerDataChanged);
        disconnect(sourceModel, &QAbstractItemModel::rowsInserted, this, &CheckProxyModel::onSourceRowsInserted);
        disconnect(sourceModel, &QAbstractItemModel::rowsRemoved, this, &CheckProxyModel::onRowsRemoved);
        disconnect(sourceModel, &QAbstractItemModel::rowsMoved, this, &CheckProxyModel::onRowsMoved);
        disconnect(sourceModel, &QAbstractItemModel::modelReset, this, &CheckProxyModel::onModelReset);
    }
    sourceModel = value;
    atLeastACheck = false;
    if(sourceModel)
    {
        connect(sourceModel, &QAbstractItemModel::dataChanged, this, &CheckProxyModel::onSourceDataChanged);
        connect(sourceModel, &QAbstractItemModel::headerDataChanged, this, &QAbstractItemModel::headerDataChanged);
        connect(sourceModel, &QAbstractItemModel::rowsInserted, this, &CheckProxyModel::onSourceRowsInserted);
        connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, &CheckProxyModel::onRowsRemoved);
        connect(sourceModel, &QAbstractItemModel::rowsMoved, this, &CheckProxyModel::onRowsMoved);
        connect(sourceModel, &QAbstractItemModel::modelReset, this, &CheckProxyModel::onModelReset);
        m_checks.fill(false, sourceModel->rowCount());
        m_checks.squeeze();
    }

    endResetModel();
}

QAbstractItemModel *CheckProxyModel::getSourceModel() const
{
    return sourceModel;
}

bool CheckProxyModel::hasAtLeastACheck() const
{
    return atLeastACheck;
}

QVector<bool> CheckProxyModel::checks() const
{
    return m_checks;
}

void CheckProxyModel::selectAll()
{
    beginResetModel();
    m_checks.fill(true);
    endResetModel();

    if(!atLeastACheck)
    {
        atLeastACheck = true;
        emit hasCheck(true);
    }
}

void CheckProxyModel::selectNone()
{
    beginResetModel();
    m_checks.fill(false);
    endResetModel();

    if(atLeastACheck)
    {
        atLeastACheck = false;
        emit hasCheck(false);
    }
}

void CheckProxyModel::onSourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>())
{
    //Map QModelIndex
    emit dataChanged(index(topLeft.row(), topLeft.column()),
                     index(bottomRight.row(), bottomRight.column()),
                     roles);
}

void CheckProxyModel::onSourceRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    beginInsertRows(QModelIndex(), first, last);
    m_checks.insert(first, last - first + 1, false);
    endInsertRows();
}

void CheckProxyModel::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);

    bool checkRemoved = false;
    for(int i = first; i < last; i++)
    {
        if(m_checks.at(i))
        {
            checkRemoved = true;
            break;
        }
    }

    beginRemoveRows(QModelIndex(), first, last);
    m_checks.remove(first, last - first + 1);
    endRemoveRows();

    if(checkRemoved)
    {
        if(atLeastACheck && !m_checks.contains(true))
        {
            //Had a check but now it doesn't anymore
            atLeastACheck = false;
            emit hasCheck(false);
        }
    }
}

void CheckProxyModel::onRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row)
{
    Q_UNUSED(parent);
    Q_UNUSED(destination);

    beginMoveRows(QModelIndex(), start, end, QModelIndex(), row);

    const int count = end - start + 1;
    for(int i = 0; i < count; i++)
    {
        m_checks.move(start + i, row + i);
    }

    endMoveRows();
}

void CheckProxyModel::onModelReset()
{
    selectNone();
}

void CheckProxyModel::setChecks(QSet<int> rowSet)
{
    beginResetModel();
    for(int r : rowSet)
    {
        if(r >= sourceModel->rowCount())
            continue;

        m_checks[r] = true;
    }
    endResetModel();

    if(!rowSet.isEmpty() && !atLeastACheck)
    {
        atLeastACheck = true;
        emit hasCheck(true);
    }

}


// New interface

//CheckProxyModel::CheckProxyModel(QObject *parent) :
//    QIdentityProxyModel(parent),
//    atLeastACheck(false)
//{
//    connect(this, &QIdentityProxyModel::modelReset, this, &CheckProxyModel::selectNone);
//    connect(this, &QIdentityProxyModel::rowsInserted, this, &CheckProxyModel::onSourceRowsIserted);
//    connect(this, &QIdentityProxyModel::rowsMoved, this, &CheckProxyModel::onSourceRowsMoved);
//    connect(this, &QIdentityProxyModel::rowsRemoved, this, &CheckProxyModel::onSourceRowsRemoved);
//}

//QVariant CheckProxyModel::data(const QModelIndex &index, int role) const
//{
//    if(index.column() == 0 && index.row() < rowCount())
//    {
//        if(role == Qt::CheckStateRole)
//        {
//            return m_checks.at(index.row()) ? Qt::Checked : Qt::Unchecked;
//        }
//    }
//    return QIdentityProxyModel::data(index, role);
//}

//bool CheckProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
//{
//    if(index.isValid() && index.row() < rowCount() && role == Qt::CheckStateRole)
//    {
//        bool val = value.value<Qt::CheckState>() == Qt::Checked;

//        m_checks[index.row()] = val;
//        //If it has already other checks don't emit signal
//        if(val && !atLeastACheck)
//        {
//            //New check
//            atLeastACheck = true;
//            emit hasCheck(true);
//        }
//        else if(atLeastACheck && !m_checks.contains(true))
//        {
//            //Had a check but now it doesn't anymore
//            atLeastACheck = false;
//            emit hasCheck(false);
//        }

//        emit dataChanged(index, index);
//        return true;
//    }
//    return false;
//}

//Qt::ItemFlags CheckProxyModel::flags(const QModelIndex &index) const
//{
//    Qt::ItemFlags f = QIdentityProxyModel::flags(index);
//    if(f != 0 && index.column() == 0)
//    {
//        f.setFlag(Qt::ItemIsUserCheckable);
//    }

//    return f;
//}

//bool CheckProxyModel::hasAtLeastACheck() const
//{
//    return atLeastACheck;
//}

//void CheckProxyModel::setChecks(QSet<int> rowSet)
//{
//    int count = rowCount();

//    if(count == 0)
//    {
//        return;
//    }

//    beginResetModel();

//    for(int r : rowSet)
//    {
//        if(r >= count)
//            continue;

//        m_checks[r] = true;
//    }

//    endResetModel();

//    if(!rowSet.isEmpty() && !atLeastACheck)
//    {
//        atLeastACheck = true;
//        emit hasCheck(true);
//    }
//}

//QVector<bool> CheckProxyModel::checks() const
//{
//    return m_checks;
//}

//void CheckProxyModel::selectAll()
//{
//    int count = rowCount();

//    beginResetModel();
//    if(count)
//    {
//        m_checks.fill(true, count);
//    }
//    else
//    {
//        m_checks.clear();
//        m_checks.squeeze();
//    }
//    endResetModel();

//    if(!atLeastACheck && count)
//    {
//        atLeastACheck = true;
//        emit hasCheck(true);
//    }
//}

//void CheckProxyModel::selectNone()
//{
//    int count = rowCount();

//    beginResetModel();
//    if(count)
//    {
//        m_checks.fill(false, count);
//    }
//    else
//    {
//        m_checks.clear();
//        m_checks.squeeze();
//    }
//    endResetModel();

//    if(atLeastACheck)
//    {
//        atLeastACheck = false;
//        emit hasCheck(false);
//    }
//}

//void CheckProxyModel::onSourceRowsIserted(QModelIndex, int first, int last)
//{
//    m_checks.insert(first, last - first + 1, false);
//}

//void CheckProxyModel::onSourceRowsMoved(QModelIndex, int start, int end, QModelIndex, int row)
//{
//    const int count = end - start + 1;
//    for(int i = 0; i < count; i++)
//    {
//        m_checks.move(start + i, row + i);
//    }
//}

//void CheckProxyModel::onSourceRowsRemoved(QModelIndex, int first, int last)
//{
//    bool checkRemoved = false;
//    for(int i = first; i < last; i++)
//    {
//        if(m_checks.at(i))
//        {
//            checkRemoved = true;
//            break;
//        }
//    }

//    m_checks.remove(first, last - first + 1);

//    if(checkRemoved)
//    {
//        if(atLeastACheck && !m_checks.contains(true))
//        {
//            //Had a check but now it doesn't anymore
//            atLeastACheck = false;
//            emit hasCheck(false);
//        }
//    }
//}
