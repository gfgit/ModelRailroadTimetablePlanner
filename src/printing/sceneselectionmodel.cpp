#include "sceneselectionmodel.h"

SceneSelectionModel::SceneSelectionModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

QVariant SceneSelectionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            switch (section)
            {
            case TypeCol:
                return tr("Type");
            case NameCol:
                return tr("Name");
            }
            break;
        }
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

int SceneSelectionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : entries.count();
}

int SceneSelectionModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant SceneSelectionModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= entries.size() || role != Qt::DisplayRole)
        return QVariant();

    const Entry& entry = entries.at(idx.row());

    switch (idx.column())
    {
    case TypeCol:
    {
        return utils::getLineGraphTypeName(entry.type);
    }
    case NameCol:
    {
        return entry.name;
    }
    }

    return QVariant();
}
