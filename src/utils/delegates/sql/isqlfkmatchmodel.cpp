#include "isqlfkmatchmodel.h"

#include <QFont>

const QString ISqlFKMatchModel::ellipsesString = QStringLiteral("...");

ISqlFKMatchModel::ISqlFKMatchModel(QObject *parent) :
    QAbstractTableModel(parent),
    hasEmptyRow(true),
    size(0)
{

}

int ISqlFKMatchModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : size;
}

int ISqlFKMatchModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

void ISqlFKMatchModel::refreshData()
{

}

void ISqlFKMatchModel::clearCache()
{

}

QString ISqlFKMatchModel::getName(db_id /*id*/) const
{
    return QString();
}

QFont initBoldFont()
{
    QFont f;
    f.setBold(true);
    f.setItalic(true);
    return f;
}

//static
QVariant ISqlFKMatchModel::boldFont()
{
    static QFont emptyRowF = initBoldFont();
    return emptyRowF;
}

void ISqlFKMatchModel::setHasEmptyRow(bool value)
{
    hasEmptyRow = value;
}
