#ifdef ENABLE_USER_QUERY

#include "sqlresultmodel.h"

#include <sqlite3pp/sqlite3pp.h>

#include "app/session.h"

SQLResultModel::SQLResultModel(QObject *parent) :
    QAbstractTableModel(parent),
    colCount(0)
{

}

SQLResultModel::~SQLResultModel()
{

}

int SQLResultModel::rowCount(const QModelIndex &parent) const
{
    return (parent.isValid() || colCount == 0) ? 0 : m_data.size() / colCount;
}

int SQLResultModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : colCount;
}

QVariant SQLResultModel::data(const QModelIndex &idx, int role) const
{
    if(!idx.isValid() || !colCount || idx.row() >= (m_data.size() / colCount))
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        int i = idx.row() * colCount + idx.column();
        return m_data.at(i);
    }

    if(role == Qt::BackgroundRole)
    {
        return backGround;
    }

    return QVariant();
}

QVariant SQLResultModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        return colNames.value(section, "ERR");
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

void SQLResultModel::setBackground(const QColor &col)
{
    beginResetModel();
    backGround = col;
    endResetModel();
}

bool SQLResultModel::initFromQuery(sqlite3pp::query *q)
{
    beginResetModel();

    colCount = q->column_count();

    colNames.clear();
    colNames.reserve(colCount);

    m_data.clear();
    m_data.squeeze();
    m_data.reserve(colCount);

    for(int i = 0; i < colCount; i++)
    {
        colNames.append(q->column_name(i));
    }

    int ret = q->step();
    while (ret == SQLITE_ROW)
    {
        for(int c = 0; c < colCount; c++)
        {
            auto r = q->getRows();
            QVariant val;
            int type = r.column_type(c);

            switch (type)
            {
            case SQLITE_INTEGER:
            {
                val = r.get<qint64>(c);
                break;
            }
            case SQLITE_TEXT:
            {
                val = r.get<QString>(c);
                break;
            }
            case SQLITE_BLOB:
            {
                val = "BLOB " + r.get<QString>(c);
                break;
            }
            case SQLITE_NULL:
            {
                val = QStringLiteral("NULL");
                break;
            }
            case SQLITE_FLOAT:
            {
                val = r.get<double>(c);
                break;
            }
            default:
                break;
            }

            m_data.append(val);
        }
        ret = q->step();
    }

    endResetModel();

    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        emit error(ret, Session->m_Db.extended_error_code(), Session->m_Db.error_msg());
        return false;
    }
    return true;
}

#endif // ENABLE_USER_QUERY
