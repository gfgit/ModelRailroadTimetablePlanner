#include "rsbackendsmodel.h"

RSImportBackendsModel::RSImportBackendsModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

RSImportBackendsModel::~RSImportBackendsModel()
{
    qDeleteAll(backends);
    backends.clear();
}

int RSImportBackendsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : backends.size();
}

QVariant RSImportBackendsModel::data(const QModelIndex &idx, int role) const
{
    if(role != Qt::DisplayRole || idx.column() != 0)
        return QVariant();

    RSImportBackend *back = getBackend(idx.row());
    if(!back)
        return QVariant();

    return back->getBackendName();
}

void RSImportBackendsModel::addBackend(RSImportBackend *backend)
{
    const int row = backends.size();
    beginInsertRows(QModelIndex(), row, row);
    backends.append(backend);
    endInsertRows();
}

RSImportBackend *RSImportBackendsModel::getBackend(int idx) const
{
    if(idx < 0 || idx >= backends.size())
        return nullptr;
    return backends.at(idx);
}
