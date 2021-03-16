#include "rsownersmatchmodel.h"

RSOwnersMatchModel::RSOwnersMatchModel(database &db, QObject *parent) :
    ISqlFKMatchModel(parent),
    mDb(db),
    q_getMatches(mDb)
{
    q_getMatches.prepare("SELECT id,name FROM rs_owners WHERE name LIKE ? LIMIT " QT_STRINGIFY(MaxMatchItems));
}

QVariant RSOwnersMatchModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= size)
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(isEmptyRow(idx.row()))
        {
            return ISqlFKMatchModel::tr("Empty");
        }
        else if(isEllipsesRow(idx.row()))
        {
            return ellipsesString;
        }

        return items[idx.row()].name;
    }
    case Qt::FontRole:
    {
        if(isEmptyRow(idx.row()))
        {
            return boldFont();
        }
        break;
    }
    }

    return QVariant();
}

void RSOwnersMatchModel::autoSuggest(const QString &text)
{
    mQuery.clear();
    if(!text.isEmpty())
    {
        mQuery.clear();
        mQuery.reserve(text.size() + 2);
        mQuery.append('%');
        mQuery.append(text.toUtf8());
        mQuery.append('%');
    }

    refreshData();
}

void RSOwnersMatchModel::refreshData()
{
    if(!mDb.db())
        return;

    beginResetModel();

    char emptyQuery = '%';

    if(mQuery.isEmpty())
        sqlite3_bind_text(q_getMatches.stmt(), 1, &emptyQuery, 1, SQLITE_STATIC);
    else
        sqlite3_bind_text(q_getMatches.stmt(), 1, mQuery, mQuery.size(), SQLITE_STATIC);


    auto end = q_getMatches.end();
    auto it = q_getMatches.begin();
    int i = 0;
    for(; i < MaxMatchItems && it != end; i++)
    {
        items[i].ownerId = (*it).get<db_id>(0);
        items[i].name = (*it).get<QString>(1);
        ++it;
    }

    size = i + 1; //Items + Empty

    if(it != end)
    {
        //There would be still rows, show Ellipses
        size++; //Items + Empty + Ellispses
    }

    q_getMatches.reset();
    endResetModel();

    emit resultsReady(false);
}

QString RSOwnersMatchModel::getName(db_id id) const
{
    if(!mDb.db())
        return QString();

    query q(mDb, "SELECT name FROM rs_owners WHERE id=?");
    q.bind(1, id);
    if(q.step() == SQLITE_ROW)
        return q.getRows().get<QString>(0);
    return QString();
}

db_id RSOwnersMatchModel::getIdAtRow(int row) const
{
    return items[row].ownerId;
}

QString RSOwnersMatchModel::getNameAtRow(int row) const
{
    return items[row].name;
}
