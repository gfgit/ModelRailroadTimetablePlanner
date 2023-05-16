/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "duplicatesimporteditemsmodel.h"

#include "../rsimportstrings.h"

#include "rollingstock/importer/intefaces/icheckname.h"

#include "utils/thread/iquittabletask.h"
#include "../intefaces/iduplicatesitemevent.h"

#include <QBrush>

#include <QElapsedTimer>

class DuplicatesImportedItemsModelTask : public IQuittableTask
{
public:
    QVector<DuplicatesImportedItemsModel::DuplicatedItem> items;
    ModelModes::Mode m_mode;

    inline DuplicatesImportedItemsModelTask(sqlite3pp::database &db, ModelModes::Mode mode, QObject *receiver) :
        IQuittableTask(receiver),
        m_mode(mode),
        mDb(db)
    {

    }

    void run() override
    {
        QElapsedTimer timer;
        timer.start();

        int count = -1;
        IDuplicatesItemModel::State state = IDuplicatesItemModel::CountingItems;

        if(wasStopped())
        {
            sendEvent(new IDuplicatesItemEvent(this,
                                               IDuplicatesItemEvent::ProgressAbortedByUser,
                                               IDuplicatesItemEvent::ProgressMaxFinished,
                                               count, state),
                      true);
            return;
        }

        //Inform model that task is started
        int max = 100;
        int progress = 0;
        sendEvent(new IDuplicatesItemEvent(this, progress, max, count, state), false);

        //Count how many items
        QByteArray sql = "SELECT COUNT(imp.id),"
                         " (CASE WHEN imp.new_name NOT NULL THEN imp.new_name ELSE imp.name END) AS name1,"
                         " (CASE WHEN dup.new_name NOT NULL THEN dup.new_name ELSE dup.name END) AS name2"
                         " FROM %1 imp"
                         " JOIN %1 dup ON dup.id<>imp.id AND %2 name1=name2"
                         " WHERE imp.match_existing_id IS NULL AND imp.import=1 AND dup.import=1";
        if(m_mode == ModelModes::Models)
        {
            sql = sql.replace("%1", "imported_rs_models");
            sql = sql.replace("%2", "imp.suffix=dup.suffix AND");
        }else{
            sql = sql.replace("%1", "imported_rs_owners");
            sql = sql.replace("%2", "  ");
        }

        query q(mDb, sql);
        q.step();
        count = q.getRows().get<int>(0);

        if(wasStopped())
        {
            sendEvent(new IDuplicatesItemEvent(this,
                                               IDuplicatesItemEvent::ProgressAbortedByUser,
                                               IDuplicatesItemEvent::ProgressMaxFinished,
                                               count, state),
                      true);
            return;
        }

        if(count == 0)
        {
            //No data to load, finish
            state = IDuplicatesItemModel::Loaded;
            sendEvent(new IDuplicatesItemEvent(this, progress, IDuplicatesItemEvent::ProgressMaxFinished,
                                               count, state),
                      true);
            return;
        }

        //Now load data
        state = IDuplicatesItemModel::LoadingData;
        max = count + 10;
        progress = 10;

        //Do not send event if the process is fast
        if(timer.elapsed() > IDuplicatesItemEvent::MinimumMSecsBeforeFirstEvent)
            sendEvent(new IDuplicatesItemEvent(this, progress, max, count, state), false);

        items.reserve(count);

        sql = "SELECT imp.id, imp.name, imp.new_name, %2"
              " (CASE WHEN imp.new_name NOT NULL THEN imp.new_name ELSE imp.name END) AS name1,"
              " (CASE WHEN dup.new_name NOT NULL THEN dup.new_name ELSE dup.name END) AS name2"
              " FROM %1 imp"
              " JOIN %1 dup ON dup.id<>imp.id AND %3 name1=name2"
              " WHERE imp.match_existing_id IS NULL AND imp.import=1 AND dup.import=1"
              " ORDER BY imp.name";
        if(m_mode == ModelModes::Models)
        {
            sql = sql.replace("%1", "imported_rs_models");
            sql = sql.replace("%2", "  ");
            sql = sql.replace("%3", "imp.suffix=dup.suffix AND");
        }else{
            sql = sql.replace("%1", "imported_rs_owners");
            sql = sql.replace("%2", "imp.sheet_idx,");
            sql = sql.replace("%3", "  ");
        }

        q.prepare(sql);

        //Send about 5 progress events during loading (but process at least 5 items between 2 events)
        const int sentTreshold = qMax(5, max / 5);

        for(auto r : q)
        {
            if(progress % 8 == 0 && wasStopped())
            {
                sendEvent(new IDuplicatesItemEvent(this,
                                                   IDuplicatesItemEvent::ProgressAbortedByUser,
                                                   IDuplicatesItemEvent::ProgressMaxFinished,
                                                   count, state),
                          true);
                return;
            }

            if(progress % sentTreshold && timer.elapsed() > IDuplicatesItemEvent::MinimumMSecsBeforeFirstEvent)
            {
                //It's time to report our progress
                sendEvent(new IDuplicatesItemEvent(this, progress, max, count, state), false);
            }

            DuplicatesImportedItemsModel::DuplicatedItem item;
            item.importedId = r.get<db_id>(0);
            item.originalName = r.get<QString>(1);
            item.customName = r.get<QString>(2);
            item.sheetIdx = m_mode == ModelModes::Owners ? r.get<int>(3) : 0;
            item.import = true;

            items.append(item);
        }

        state = IDuplicatesItemModel::Loaded;
        sendEvent(new IDuplicatesItemEvent(this, progress, IDuplicatesItemEvent::ProgressMaxFinished,
                                           count, state),
                  true);
    }

private:
    sqlite3pp::database &mDb;
};

DuplicatesImportedItemsModel::DuplicatesImportedItemsModel(ModelModes::Mode mode, database &db, ICheckName *i, QObject *parent):
    IDuplicatesItemModel(db, parent),
    iface(i),
    m_mode(mode)
{
}

QVariant DuplicatesImportedItemsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        if(m_mode == ModelModes::Models && section >= SheetIdx)
            section++; //Skip SheetIdx for Models

        switch (section)
        {
        case Import:
            return RsImportStrings::tr("Import");
        case SheetIdx:
            return RsImportStrings::tr("Sheet No.");
        case Name:
            return RsImportStrings::tr("Name");
        case CustomName:
            return RsImportStrings::tr("Custom Name");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

int DuplicatesImportedItemsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : items.size();
}

int DuplicatesImportedItemsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_mode == ModelModes::Models ? NCols - 1 : NCols;
}

QVariant DuplicatesImportedItemsModel::data(const QModelIndex &idx, int role) const
{
    int col = idx.column();
    if(m_mode == ModelModes::Models && col >= SheetIdx)
        col++; //Skip SheetIdx for Models

    if (!idx.isValid() || idx.row() >= items.size() || col >= NCols)
        return QVariant();

    const DuplicatedItem& item = items.at(idx.row());

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (col)
        {
        case SheetIdx:
            return item.sheetIdx;
        case Name:
            return item.originalName;
        case CustomName:
            return item.customName;
        }
        break;
    }
    case Qt::EditRole:
    {
        if(col == CustomName)
            return item.customName;
        break;
    }
    case Qt::BackgroundRole:
    {
        if(!item.import || (idx.column() == CustomName && item.customName.isEmpty()))
            return QBrush(Qt::lightGray);
        break;
    }
    case Qt::CheckStateRole:
    {
        if(col == Import)
            return item.import ? Qt::Checked : Qt::Unchecked;
        break;
    }
    }
    return QVariant();
}

bool DuplicatesImportedItemsModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    int col = idx.column();
    if(m_mode == ModelModes::Models && col >= SheetIdx)
        col++; //Skip SheetIdx for Models

    if (!idx.isValid() || idx.row() >= items.size())
        return false;

    QModelIndex first = idx;
    QModelIndex last = idx;

    DuplicatedItem& item =items[idx.row()];

    switch (role)
    {
    case Qt::EditRole:
    {
        if(col == CustomName)
        {
            QString newName = value.toString().simplified();
            if(item.customName == newName)
                return false;

            QString errText;
            if(!iface->checkCustomNameValid(item.importedId, item.originalName, newName, &errText))
            {
                emit error(errText);
                return false;
            }

            const char *sql[NModes] = {
                "UPDATE imported_rs_owners SET new_name=? WHERE id=?",

                "UPDATE imported_rs_models SET new_name=? WHERE id=?"
            };

            command set_name(mDb, sql[m_mode]);
            set_name.bind(1, newName);
            set_name.bind(2, item.importedId);
            if(set_name.execute() != SQLITE_OK)
                return false;

            item.customName = newName;
        }
        break;
    }
    case Qt::CheckStateRole:
    {
        if(col == Import)
        {
            Qt::CheckState cs = value.value<Qt::CheckState>();
            const bool import = cs == Qt::Checked;
            if(item.import == import)
                return false; //No change

            if(import)
            {
                //Newly imported, check if there are duplicates
                QString errText;
                if(!iface->checkCustomNameValid(item.importedId, item.originalName, item.customName, &errText))
                {
                    emit error(errText);
                    return false;
                }
            }

            const char *sql[NModes] = {
                "UPDATE imported_rs_owners SET import=? WHERE id=?",

                "UPDATE imported_rs_models SET import=? WHERE id=?"
            };

            command set_imported(mDb, sql[m_mode]);
            set_imported.bind(1, import ? 1 : 0);
            set_imported.bind(2, item.importedId);
            if(set_imported.execute() != SQLITE_OK)
                return false;

            item.import = import;

            //Update all columns to update background
            first = index(idx.row(), 0);
            //Do not use NCols because in Models mode SheetIdx is hidden
            last = index(idx.row(), columnCount());
        }
        break;
    }
    }

    emit dataChanged(first, last);
    return true;
}

Qt::ItemFlags DuplicatesImportedItemsModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
    int col = idx.column();
    if(m_mode == ModelModes::Models && col >= SheetIdx)
        col++; //Skip SheetIdx for Models
    if(col == Import)
        f.setFlag(Qt::ItemIsUserCheckable);
    if(col == CustomName)
        f.setFlag(Qt::ItemIsEditable);

    return f;
}

IQuittableTask *DuplicatesImportedItemsModel::createTask(int mode)
{
    Q_UNUSED(mode) //Only 1 mode possible
    return new DuplicatesImportedItemsModelTask(mDb, m_mode, this);
}

void DuplicatesImportedItemsModel::handleResult(IQuittableTask *task)
{
    beginResetModel();
    items = static_cast<DuplicatesImportedItemsModelTask *>(task)->items;
    endResetModel();
}
