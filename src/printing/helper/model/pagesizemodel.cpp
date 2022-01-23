#include "pagesizemodel.h"

#include <QPageSize>

PageSizeModel::PageSizeModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

int PageSizeModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : QPageSize::NPageSize;
}

QVariant PageSizeModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || role != Qt::DisplayRole)
        return QVariant();

    QPageSize::PageSizeId pageId = QPageSize::PageSizeId(idx.row());
    return QPageSize::name(pageId);
}
